#include "network_win.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>
#include "grpc/route_guide.grpc.pb.h"

#define LISTENING_ADDRESS "0.0.0.0"
#define network_PORT 8000
//#define SERVER_ADDRESS "192.168.1.30"
#define SERVER_ADDRESS "127.0.0.1"
#define BUFFER_SIZE 800000
#define MAX_PACKET_SIZE 4000


using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using gamingstreaming::Frame;
using gamingstreaming::InputCommand;

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using gamingstreaming::Frame;
using gamingstreaming::InputCommand;


Frame MakeFrame(const int height, const int width, const std::vector<int> &pixels) {
    Frame f;
    f.set_height(height);
    f.set_width(width);
    for (const int pixel: pixels) {
        f.add_pixels(pixel);
    }
    return f;
}


//class GamingStreamingServiceImpl final : public gamingstreaming::GamingStreamingService::Service {
//public:
//    explicit GamingStreamingServiceImpl() {
//    }
//
//    Status GamingChannel(ServerContext *context,
//                         ServerReaderWriter<Frame, InputCommand> *stream) override {
//        // std::vector<Frame> new_frames;
//        std::vector<int> fakePixels;
//        for (int i=0; i< 400000; i++) {
//            fakePixels.push_back(i);
//        }
//
//
//        std::vector<Frame> new_frames{
//                MakeFrame(100, 100, fakePixels),
//                MakeFrame(100, 100, fakePixels),
//                MakeFrame(100, 100, fakePixels),
//                MakeFrame(100, 100, fakePixels)
//        };
//
//        InputCommand cmd;
//        while (stream->Read(&cmd)) {
//            std::cout << "Received an input command" << cmd.command() << std::endl;
//            for (const Frame &frame : new_frames) {
//                stream->Write(frame);
//            }
//        }
//
//        return Status::OK;
//    }
//
//private:
//};
//
//void RunServer() {
//    std::string server_address("0.0.0.0:50051");
//    GamingStreamingServiceImpl service;
//
//    ServerBuilder builder;
//    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
//    builder.RegisterService(&service);
//    std::unique_ptr<Server> server(builder.BuildAndStart());
//    std::cout << "Server listening on " << server_address << std::endl;
//    server->Wait();
//}

//InputCommand MakeInputCommand(const std::string& command) {
//    InputCommand c;
//    c.set_command(command);
//    return c;
//}


//class GamingStreamingServiceClient {
//public:
//    GamingStreamingServiceClient(std::shared_ptr<Channel> channel)
//            : stub_(gamingstreaming::GamingStreamingService::NewStub(channel)) {
//    }
//
//
//    void GamingChannel() {
//        ClientContext context;
//
//        std::shared_ptr<ClientReaderWriter<InputCommand, Frame> > stream(
//                stub_->GamingChannel(&context));
//
//        std::thread writer([stream]() {
//            std::vector<InputCommand> input_commands {
//                    MakeInputCommand("click (0,100)"),
//                    MakeInputCommand("click (100,0)"),
//                    MakeInputCommand("click (100,100)"),
//                    MakeInputCommand("click (100,50)")};
//            for (const InputCommand& cmd : input_commands) {
//                std::cout << "Sending command " << cmd.command() << std::endl;
//                stream->Write(cmd);
//            }
//            stream->WritesDone();
//        });
//
//        Frame server_frame;
//        while (stream->Read(&server_frame)) {
//            std::cout << "Got frame (" << server_frame.height()
//                      << ", " << server_frame.width() << ") " << std::endl;
//        }
//        writer.join();
//        Status status = stream->Finish();
//        if (!status.ok()) {
//            std::cout << "GamingStreaming rpc failed." << std::endl;
//        }
//    }
//
//private:
//
//    const float kCoordFactor_ = 10000000.0;
//    std::unique_ptr<gamingstreaming::GamingStreamingService::Stub> stub_;
//};

int packet_sender_thread(void *arg) {
    StreamingEnvironment *se = (StreamingEnvironment*)arg;

    while (! se->finishing) {

    }

    return 0;
}

int packet_receiver_thread(void *arg) {
    StreamingEnvironment *se = (StreamingEnvironment*)arg;

    while (! se->finishing) {

    }

    return 0;
}