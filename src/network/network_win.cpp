#include "network_win.h"

#define LISTENING_ADDRESS "0.0.0.0"
#define network_PORT 8000
#define SERVER_ADDRESS "127.0.0.1"
#define BUFFER_SIZE 800000
#define MAX_PACKET_SIZE 3000


long compute_quick_n_dirty_hash(char* array, long length) {
    long result = 0;
    int modulo = 10000;
    for(int i=0; i<length; i++) {
        result += array[i];
        result = (result * result) % modulo;
    }
    return result;
}

int serialize_packet_data(packet_data* src, int8_t* dst) {
    memcpy(dst, src, sizeof(packet_data));
    memcpy(dst + sizeof(packet_data), src->data, src->size);
    return sizeof(packet_data) + src->size;
}

void deserialize_packet_data(const int8_t* src, packet_data* dst) {
    memcpy(dst, src, sizeof(packet_data));
    memcpy(dst->data , src, dst->size);
}

void do_session(udp::socket& socket, udp::endpoint endpoint, StreamingEnvironment* se)
{
    int packet_count = 0;
    int8_t* c_buffer = new int8_t[BUFFER_SIZE];
    int8_t *temp_buffer = new int8_t[BUFFER_SIZE];
    while (! se->finishing) {
        // Allocate a buffer

        packet_data *pkt_d = (packet_data *) simple_queue_pop(se->packet_sender_thread_queue);

        std::chrono::system_clock::time_point t1 = std::chrono::system_clock::now();
        int data_length = serialize_packet_data(pkt_d, c_buffer);
        std::chrono::system_clock::time_point t2 = std::chrono::system_clock::now();
        long hash = compute_quick_n_dirty_hash((char*) c_buffer, data_length);
        std::chrono::system_clock::time_point t3 = std::chrono::system_clock::now();
//        log_info("[network] I will send a packet: %d bytes (hash: %d) (dst->size: %d)", data_length, hash, pkt_d->size);
        int max_packet_count = data_length / MAX_PACKET_SIZE + (data_length % MAX_PACKET_SIZE != 0);;
        long already_copied_bytes_count = 0;
        std::chrono::system_clock::time_point t4 = std::chrono::system_clock::now();
        for (int i = 1; i <= max_packet_count; i++) {
            int payload_size = MAX_PACKET_SIZE;
            if (i == max_packet_count && (data_length % MAX_PACKET_SIZE > 0)) {
                payload_size = (data_length % MAX_PACKET_SIZE);
            }
            int udp_packet_size = payload_size + 5 * sizeof(int);
            ((int *) temp_buffer)[0] = i;
            ((int *) temp_buffer)[1] = max_packet_count;
            ((int *) temp_buffer)[2] = packet_count;
            ((int *) temp_buffer)[3] = data_length;
            ((int *) temp_buffer)[4] = already_copied_bytes_count;
            memcpy(temp_buffer + 5 * sizeof(int), c_buffer + already_copied_bytes_count, payload_size);

            socket.async_send_to(
                boost::asio::buffer(temp_buffer, udp_packet_size), endpoint,
                [](boost::system::error_code /*ec*/, std::size_t /*bytes_sent*/) {
//                    free(temp_buffer);
                }
            );
//            free(temp_buffer);
            long packet_hash = compute_quick_n_dirty_hash((char*) temp_buffer, udp_packet_size);
//            log_info("[network]    - subpacket [%d %d/%d] sent: %d bytes (hash: %d)", packet_count, i, max_packet_count, udp_packet_size, packet_hash);
            already_copied_bytes_count += payload_size;
        }

        free(pkt_d->data);
        free(pkt_d);
        
        std::chrono::system_clock::time_point t5 = std::chrono::system_clock::now();
        //log_info("[network] sent %d bytes", data_length);
       float d1 = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() / 1000.0;
       float d2 = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count() / 1000.0;
       float d3 = std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count() / 1000.0;
       float d4 = std::chrono::duration_cast<std::chrono::microseconds>(t5 - t4).count() / 1000.0;
    //    log_info(" - t1: %f ms", d1);
    //    log_info(" - t2: %f ms", d2);
    //    log_info(" - t3: %f ms", d3);
    //    log_info(" - t3: %f ms", d4);
        packet_count = (packet_count + 1) % 3000;
    }
    free(c_buffer);
}

int packet_sender_thread(void *arg) {
	StreamingEnvironment *se = (StreamingEnvironment*)arg;

    try
    {
        auto const address = boost::asio::ip::make_address(LISTENING_ADDRESS);
        auto const port = static_cast<unsigned short>(network_PORT);

        // The io_context is required for all I/O
        boost::asio::io_context ioc{1};

        udp::socket sock(ioc, udp::endpoint(udp::v4(), port));
        for (;;)
        {
            char data[BUFFER_SIZE];
            udp::endpoint sender_endpoint;
            size_t length = sock.receive_from(boost::asio::buffer(data, BUFFER_SIZE), sender_endpoint);
            do_session(sock, sender_endpoint, se);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

	return 0;
}

typedef struct map_packet_entry {
    int packet_number;
    int total_size;
    int copied_bytes;
//    int8_t* c_buffer;
    std::vector<int8_t> bytes;
    int expected_packet_count;
    int received_packet_count;
//    SimpleQueue *processed_packets;
} map_packet_entry;


fifo_map<int, map_packet_entry*> map_of_incoming_buffers;

void flush_packets(StreamingEnvironment *se, fifo_map<int, map_packet_entry *> &m, int last_packet_number) {

    for(auto iter = m.cbegin(); iter != m.cend();) {
        int packet_number =  iter->first;
        map_packet_entry* map_entry = iter->second;

        std::chrono::system_clock::time_point t2 = std::chrono::system_clock::now();

        if (map_entry->expected_packet_count != map_entry->received_packet_count) {
            continue;
        }

        long hash = compute_quick_n_dirty_hash((char *) map_entry->bytes.data(), map_entry->total_size);
//             log_info("[network] received a packet: %d bytes (hash: %d)", map_entry->total_size, hash);

        packet_data *network_packet_data = new packet_data();
        network_packet_data->size = map_entry->total_size;
        network_packet_data->data = new uint8_t[map_entry->total_size - sizeof(packet_data)];
//            network_packet_data->data = (uint8_t*) malloc(sizeof(uint8_t) * 200000);//new uint8_t[map_entry->total_size - sizeof(packet_data)];
        std::chrono::system_clock::time_point t3 = std::chrono::system_clock::now();


//            free(network_packet_data->data);

//            memcpy(network_packet_data, map_entry->bytes.data(), sizeof(packet_data));
        network_packet_data->size = map_entry->total_size - sizeof(packet_data);
        memcpy(network_packet_data->data, map_entry->bytes.data() + sizeof(packet_data), map_entry->total_size - sizeof(packet_data));
//            free(network_packet_data->data);
//            free(network_packet_data);

//             log_info("vv");
//            deserialize_packet_data(map_entry->bytes.data(), network_packet_data);
        std::chrono::system_clock::time_point t4 = std::chrono::system_clock::now();

//             log_info("ww");
        simple_queue_push(se->network_simulated_queue, network_packet_data);
        std::chrono::system_clock::time_point t5 = std::chrono::system_clock::now();

        if (packet_number > last_packet_number) {
            continue;
        }

        map_packet_entry* entry = iter->second;
        free(entry);
        
        m.erase(iter++);
    }
}

int packet_receiver_thread(void *arg) {
	StreamingEnvironment *se = (StreamingEnvironment*)arg;

    auto const host = SERVER_ADDRESS;
    auto const port = "8000";
    auto const text = "HelloWorld from a network client!";

    usleep(3 * 1000);

    // The io_service is required for all I/O
    boost::asio::io_service ios;

    // These objects perform our I/O
    boost::asio::io_context io_context;

    udp::socket s(io_context, udp::endpoint(udp::v4(), 0));

    udp::resolver resolver(io_context);
    udp::resolver::results_type endpoints = resolver.resolve(udp::v4(), host, port);

    // Send the message
    std::chrono::system_clock::time_point t0a = std::chrono::system_clock::now();
    s.send_to(boost::asio::buffer(std::string(text)), *endpoints.begin());
    std::chrono::system_clock::time_point t0b = std::chrono::system_clock::now();
    float d0 = std::chrono::duration_cast<std::chrono::microseconds>(t0b - t0a).count() / 1000.0;
    log_info(" - d0 %f ms", d0);

    int8_t reply[BUFFER_SIZE];

    udp::endpoint sender_endpoint;
    int packet_count = 0;
    while (!se->finishing) {
//         log_info("aa");

        std::chrono::system_clock::time_point t1 = std::chrono::system_clock::now();
//        while (loop) {
        auto buffer_reply = boost::asio::buffer(reply, BUFFER_SIZE);
        size_t reply_length = s.receive_from(buffer_reply, sender_endpoint);
//         log_info("bb");

        int8_t* data = (int8_t*) buffer_reply.data();
        int8_t* payload_address = data + 5 * sizeof(int);
        long payload_size = reply_length - 5 * sizeof(int);
        int packet_index = ((int *) data)[0];
        int expected_packet_count = ((int *) data)[1];
        int packet_number = ((int *) data)[2];
        int packet_data_size = ((int *) data)[3];
        int offset = ((int *) data)[4];

//        log_info(" >> %d[%d]", packet_number);

        if (map_of_incoming_buffers.find(packet_number) == map_of_incoming_buffers.end()) {
//             log_info("bb1");
            map_packet_entry *new_entry = new map_packet_entry();
            new_entry->packet_number = packet_number;
//            new_entry->c_buffer = new int8_t[packet_data_size];
            new_entry->total_size = packet_data_size;
            new_entry->copied_bytes = 0;
            new_entry->expected_packet_count = expected_packet_count;
            new_entry->received_packet_count = 0;
            map_of_incoming_buffers[packet_number] = new_entry;
//             log_info("bb2");
        }
//         log_info("cc");

        map_packet_entry *map_entry = map_of_incoming_buffers[packet_number];

        std::vector<uint8_t> v(payload_address, payload_address + payload_size);
        map_entry->bytes.insert(map_entry->bytes.end(), v.begin(), v.end());
        v.erase(v.begin(), v.end());

//        memcpy(map_entry->c_buffer + offset, payload_address, payload_size);
        map_entry->copied_bytes += payload_size;
        map_entry->received_packet_count += 1;

//        log_info("[receive] Payload: %d", payload_size);
//        simple_queue_push(map_entry->processed_packets, &packet_index);
//         log_info("dd");

        long packet_hash = compute_quick_n_dirty_hash((char*) data, reply_length);
//         log_info("[network] read sub packet [%d %d/%d]: %d bytes (hash: %d)", packet_number, packet_index, map_entry->expected_packet_count, reply_length, packet_hash);

        if (map_entry->received_packet_count == expected_packet_count) {
            if(packet_number > packet_count) {
                // A packet may have been dropped!
                log_info("A packet may have been dropped...");
            }

            if (payload_size == 0) {
//                log_info("aborting: payload == 0");
                continue;
            }




//             log_info("[Network] frame received");
        //     float d1 = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() / 1000.0;
        //     float d2 = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count() / 1000.0;
        //     float d3 = std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count() / 1000.0;
        //     float d4 = std::chrono::duration_cast<std::chrono::microseconds>(t5 - t4).count() / 1000.0;
        //    log_info(" - r.d1 %f ms", d1);
        //    log_info(" - r.d2 %f ms", d2);
        //    log_info(" - r.d3 %f ms", d3);
        //    log_info(" - r.d4 %f ms", d4);

            packet_count = (packet_count + 1) % 3000;
//             log_info("yy");

            // Clean the map entry
            // while (simple_queue_length(map_entry->processed_packets) > 0) {
            //     FrameData* frame_data = (FrameData*)simple_queue_pop(map_entry->processed_packets);
            // }
            // if (map_of_incoming_buffers.find(packet_number) != map_of_incoming_buffers.end()) {
            //     map_of_incoming_buffers.erase(packet_number);
            // }
            // free(map_entry->processed_packets);
            // free(map_entry->c_buffer);
            // free(map_entry);
            flush_packets(se, map_of_incoming_buffers, packet_number);
//             log_info("zz");
        }
//         log_info("czzc");
    }

    // If we get here then the connection is closed gracefully
	return 0;
}