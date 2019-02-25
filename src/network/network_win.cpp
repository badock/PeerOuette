#include "network_win.h"

#define USE_NETWORK false

#define LISTENING_ADDRESS "0.0.0.0"
#define WEBSOCKET_PORT 8000
#define SERVER_ADDRESS "127.0.0.1"
#define BUFFER_SIZE 900000


int serialize_packet_data(packet_data* src, int8_t* dst) {
    memcpy(dst, src, sizeof(packet_data));
    memcpy(dst + sizeof(packet_data), src->data, src->size);
    return sizeof(packet_data) + src->size;
}

void deserialize_packet_data(const int8_t* src, packet_data* dst) {
    memcpy(dst, src, sizeof(packet_data));
    memcpy(dst->data , src + sizeof(packet_data), dst->size);
}

void do_session(tcp::socket& socket, StreamingEnvironment* se)
{
    try
    {
        // Construct the stream by moving in the socket
        websocket::stream<tcp::socket> ws{std::move(socket)};

        // Accept the websocket handshake
        ws.accept();

        // This buffer will hold the incoming message
        boost::beast::multi_buffer buffer;

        // Read a message
        ws.read(buffer);
        ws.binary(true);

        // Allocate a buffer
        int8_t* c_buffer = (int8_t*) malloc(BUFFER_SIZE * sizeof(int8_t));

        while (! se->finishing) {
            packet_data* pkt_d = (packet_data*) simple_queue_pop(se->packet_sender_thread_queue);
            int data_length = serialize_packet_data(pkt_d, c_buffer);
            std::vector<int8_t> data(c_buffer, c_buffer + data_length);
            auto buffer = boost::asio::buffer(data, data_length);
            int n_bytes_sent = ws.write(buffer);
            log_info("[Websocket] sent %d bytes", n_bytes_sent);
        }
    }
    catch(boost::system::system_error const& se)
    {
        // This indicates that the session was closed
        if(se.code() != websocket::error::closed)
            std::cerr << "Error: " << se.code().message() << std::endl;
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int packet_sender_thread(void *arg) {
	StreamingEnvironment *se = (StreamingEnvironment*)arg;

    try
    {
        auto const address = boost::asio::ip::make_address(LISTENING_ADDRESS);
        auto const port = static_cast<unsigned short>(WEBSOCKET_PORT);

        // The io_context is required for all I/O
        boost::asio::io_context ioc{1};

        // The acceptor receives incoming connections
        tcp::acceptor acceptor{ioc, {address, port}};
        for(;;)
        {
            // This will receive the new connection
            tcp::socket socket{ioc};

            // Block until we get a connection
            acceptor.accept(socket);

            // Launch the session, transferring ownership of the socket
            std::thread{std::bind(
                    &do_session,
                    std::move(socket),
                    se)}.detach();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

	return 0;
}

int packet_receiver_thread(void *arg) {
	StreamingEnvironment *se = (StreamingEnvironment*)arg;

    auto const host = SERVER_ADDRESS;
    auto const port = "8000";
    auto const text = "HelloWorld from a websocket client!";

    // The io_service is required for all I/O
    boost::asio::io_service ios;

    // These objects perform our I/O
    tcp::resolver resolver{ios};
    websocket::stream<tcp::socket> ws{ios};

    // Look up the domain name
    auto const lookup = resolver.resolve({host, port});

    // Make the connection on the IP address we get from a lookup
    boost::asio::connect(ws.next_layer(), lookup);

    // Perform the websocket handshake
    ws.handshake(host, "/");

    // Send the message
    ws.write(boost::asio::buffer(std::string(text)));

    // This buffer will hold the incoming message
    boost::beast::multi_buffer buffer;

    // Allocate a buffer
    int8_t* c_buffer = (int8_t *) malloc(BUFFER_SIZE * sizeof(int8_t));
    ws.binary(true);

    while (!se->finishing) {
        // Read a message into our buffer
        ws.read(buffer);

        log_info("[websocket] read %d bytes", buffer.size());

        packet_data *network_packet_data = (packet_data*) malloc(sizeof(packet_data));
        network_packet_data->data = (uint8_t *) malloc(sizeof(uint8_t) * buffer.size() - sizeof(packet_data));

        int8_t * tempchar = new int8_t[buffer.size()];
        boost::asio::buffer_copy(boost::asio::buffer(tempchar, buffer.size()), buffer.data(), buffer.size());

        deserialize_packet_data(tempchar, network_packet_data);

        simple_queue_push(se->network_simulated_queue, network_packet_data);

        // clear buffer
        buffer.consume(buffer.size());
    }

    // Close the WebSocket connection
    ws.close(websocket::close_code::normal);

    // If we get here then the connection is closed gracefully

	return 0;
}