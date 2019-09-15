#include "network.h"

#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include "protos/route_guide.grpc.pb.h"
#include "src/inputs/inputs.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using gamingstreaming::FrameSubPacket;
using gamingstreaming::InputCommand;

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using gamingstreaming::InputCommand;

using namespace std;

FrameSubPacket MakeFrame(const int frame_number,
                         const int packet_number,
                         const int size,
                         const int64_t pts,
                         const int64_t dts,
                         const int flags,
                         const char* data,
                         const int mouse_x,
                         const int mouse_y,
                         const bool mouse_is_visible,
                         const int flow_id,
                         const int height,
                         const int width) {
    FrameSubPacket f;
    f.set_frame_number(frame_number);
    f.set_packet_number(packet_number);
    f.set_size(size);
    f.set_pts(pts);
    f.set_dts(dts);
    f.set_flags(flags);
    f.set_data(data, size);
    // fields related to mouse
    f.set_mouse_x(mouse_x);
    f.set_mouse_y(mouse_y);
    f.set_mouse_is_visible(mouse_is_visible);
    f.set_flow_id(flow_id);
    f.set_height(height);
    f.set_width(width);

    return f;
}

class GamingStreamingServiceImpl final : public gamingstreaming::GamingStreamingService::Service {
public:
    StreamingEnvironment* se;

    explicit GamingStreamingServiceImpl(StreamingEnvironment* se) {
        this->se = se;
    }

    Status GamingChannel(ServerContext *context,
                         ServerReaderWriter<FrameSubPacket, InputCommand> *stream) override {
        InputCommand cmd;

        se->client_connected = true;

        StreamingEnvironment * streaming_environment = se;

        std::thread writer([stream, streaming_environment]() {
            InputCommand cmd;
            while (stream->Read(&cmd)) {
                if (cmd.command().compare("BEGIN_STREAM") == 0) {
                    streaming_environment->can_begin_stream = true;
                } else {
                    simulate_input_event(&cmd);
                }
            }
        });

        while (!se->can_begin_stream) {
            usleep(30 * 1000);
        }

        while (!streaming_environment->finishing && streaming_environment->client_connected) {
            const auto pkt_d = streaming_environment->packet_sender_thread_queue.pop();

            mouse_info* mouse_info_ptr = get_mouse_info();

            FrameSubPacket subPacket = MakeFrame(pkt_d->frame_number,
                                                 pkt_d->packet_number,
                                                 pkt_d->size,
                                                 pkt_d->pts,
                                                 pkt_d->dts,
                                                 pkt_d->flags,
                                                 (char*) pkt_d->data,
                                                 mouse_info_ptr->x,
                                                 mouse_info_ptr->y,
                                                 mouse_info_ptr->visible,
                                                 se->flow_id,
                                                 se->height,
                                                 se->width);
            stream->Write(subPacket);

            free(mouse_info_ptr);

            free(pkt_d->data);
            free(pkt_d);
        }

        return Status::OK;
    }

private:
};

InputCommand MakeInputCommand(const std::string& command) {
    InputCommand c;
    c.set_command(command);
    return c;
}


class GamingStreamingServiceClient {
public:
    StreamingEnvironment* se;
    GamingStreamingServiceClient(const std::shared_ptr<Channel> &channel, StreamingEnvironment* se)
            : stub_(gamingstreaming::GamingStreamingService::NewStub(channel)) {
        this->se = se;
    }


    void GamingChannel() {
        ClientContext context;

        std::shared_ptr<ClientReaderWriter<InputCommand, FrameSubPacket>> stream(stub_->GamingChannel(&context));

        StreamingEnvironment* streaming_environment = se;
        std::thread writer([stream, streaming_environment]() {
            stream->Write(MakeInputCommand("BEGIN_STREAM"));
            while (!streaming_environment->finishing) {
                InputCommand* ci = streaming_environment->input_command_queue.pop();
                stream->Write(*ci);
                free(ci);
            }
        });

        FrameSubPacket server_frame;
        while (stream->Read(&server_frame) && !se->finishing) {

            auto new_packet_data = new packet_data();
            new_packet_data->frame_number = server_frame.frame_number();
            new_packet_data->packet_number = server_frame.packet_number();
            new_packet_data->size = server_frame.size();
            new_packet_data->flags = server_frame.flags();
            new_packet_data->dts = server_frame.dts();
            new_packet_data->pts = server_frame.pts();

            // refresh mouse cursor position
            se->client_mouse_x = server_frame.mouse_x();
            se->client_mouse_y = server_frame.mouse_y();
            se->client_mouse_is_visible = server_frame.mouse_is_visible();

            // Copy data
            new_packet_data->data = new uint8_t[new_packet_data->size];
            memcpy(new_packet_data->data, server_frame.data().data(), new_packet_data->size);

            se->network_simulated_queue.push(new_packet_data);
        }

        Status status = stream->Finish();
        if (!status.ok()) {
            std::cout << "GamingStreaming rpc failed." << std::endl;
        }
    }

private:
    std::unique_ptr<gamingstreaming::GamingStreamingService::Stub> stub_;
};

int packet_sender_thread(void *arg) {
    auto se = (StreamingEnvironment*)arg;

    std::string server_address(se->listen_address);
    GamingStreamingServiceImpl service(se);

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    se->server_initialized = true;
    server->Wait();

    return 0;
}

int packet_receiver_thread(void *arg) {
    auto se = (StreamingEnvironment*) arg;

    GamingStreamingServiceClient client(
            grpc::CreateChannel(se->server_address,
                                grpc::InsecureChannelCredentials()),
            se
    );
    client.GamingChannel();

    return 0;
}