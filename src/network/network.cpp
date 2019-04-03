#include "network.h"

#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include "grpc/route_guide.grpc.pb.h"

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
                         const char* data) {
    FrameSubPacket f;
    f.set_frame_number(frame_number);
    f.set_packet_number(packet_number);
    f.set_size(size);
    f.set_pts(pts);
    f.set_dts(dts);
    f.set_flags(flags);
    f.set_data(data, size);

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
        bool can_begin_stream = false;

        while (!can_begin_stream) {
            stream->Read(&cmd);
            if (cmd.command().compare("BEGIN_STREAM") == 0) {
                can_begin_stream = true;
            }
        }

        while (!se->finishing && se->client_connected) {
            const auto pkt_d = se->packet_sender_thread_queue.pop();

            FrameSubPacket subPacket = MakeFrame(pkt_d->frame_number,
                                                 pkt_d->packet_number,
                                                 pkt_d->size,
                                                 pkt_d->pts,
                                                 pkt_d->dts,
                                                 pkt_d->flags,
                                                 (char*) pkt_d->data);
            stream->Write(subPacket);

            free(pkt_d->data);
            free(pkt_d);

            while (stream->Read(&cmd)) {
                std::cout << "Received an input command" << cmd.command() << std::endl;
            }
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

        std::shared_ptr<ClientReaderWriter<InputCommand, FrameSubPacket> >
                stream(
                stub_->GamingChannel(&context));

        std::thread writer([stream]() {
            stream->Write(MakeInputCommand("BEGIN_STREAM"));
            stream->WritesDone();
        });

        FrameSubPacket server_frame;
        while (stream->Read(&server_frame) && !se->finishing) {
//            std::cout << "Got frame (" << server_frame.frame_number()
//                      << ", " << server_frame.packet_number() << ") " << std::endl;

            auto new_packet_data = new packet_data();
            new_packet_data->frame_number = server_frame.frame_number();
            new_packet_data->packet_number = server_frame.packet_number();
            new_packet_data->size = server_frame.size();
            new_packet_data->flags = server_frame.flags();
            new_packet_data->dts = server_frame.dts();
            new_packet_data->pts = server_frame.pts();

            // Copy data
            new_packet_data->data = new uint8_t[new_packet_data->size];
            memcpy(new_packet_data->data, server_frame.data().data(), new_packet_data->size);

            se->network_simulated_queue.push(new_packet_data);
        }
//        writer.join();
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