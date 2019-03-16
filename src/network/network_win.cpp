#include "network_win.h"

#define LISTENING_ADDRESS "0.0.0.0"
#define network_PORT 8000
#define SERVER_ADDRESS "192.168.1.30"
#define BUFFER_SIZE 800000
#define MAX_PACKET_SIZE 50000


long compute_quick_n_dirty_hash(char* array, long length) {
//    return -1;
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

void handler(int8_t* data)
{
    log_info("error??");
}

void do_session(udp::socket& socket, udp::endpoint endpoint, StreamingEnvironment* se)
{
    int packet_count = 0;
    // int8_t* c_buffer = new int8_t[BUFFER_SIZE];
    std::vector<int8_t> c_buffer(BUFFER_SIZE, 0);
    int8_t *temp_buffer = new int8_t[BUFFER_SIZE];
    while (! se->finishing) {
        packet_data *pkt_d = (packet_data *) simple_queue_pop(se->packet_sender_thread_queue);

        if (pkt_d->size + sizeof(packet_data) > c_buffer.size()) {
            c_buffer.resize(pkt_d->size + sizeof(packet_data));
        }

        std::chrono::system_clock::time_point t1 = std::chrono::system_clock::now();
        int data_length = serialize_packet_data(pkt_d, c_buffer.data());
        std::chrono::system_clock::time_point t2 = std::chrono::system_clock::now();
        long hash = compute_quick_n_dirty_hash((char*) c_buffer.data(), data_length);
        std::chrono::system_clock::time_point t3 = std::chrono::system_clock::now();
        // log_info("[network] I will send a packet: %d bytes (hash: %d) (dst->size: %d)", data_length, hash, pkt_d->size);
        int max_packet_count = data_length / MAX_PACKET_SIZE + (data_length % MAX_PACKET_SIZE != 0);;
        long already_copied_bytes_count = 0;
        std::chrono::system_clock::time_point t4 = std::chrono::system_clock::now();

        for (int i = 1; i <= max_packet_count; i++) {
            int payload_size = MAX_PACKET_SIZE;

            int udp_packet_size = payload_size + 5 * sizeof(int);
            ((int *) temp_buffer)[0] = i;
            ((int *) temp_buffer)[1] = max_packet_count;
            ((int *) temp_buffer)[2] = packet_count;
            ((int *) temp_buffer)[3] = data_length;
            ((int *) temp_buffer)[4] = already_copied_bytes_count;
            memcpy(temp_buffer + 5 * sizeof(int), c_buffer.data() + already_copied_bytes_count, payload_size);

            socket.async_send_to(
                boost::asio::buffer(temp_buffer, udp_packet_size), endpoint, boost::bind(handler, temp_buffer)
            );

            if (data_length > 150000) {
                usleep(1200);
            }

            long packet_hash = compute_quick_n_dirty_hash((char*) temp_buffer, udp_packet_size);
             log_info("[network]    - subpacket [%d %d/%d] sent: %d bytes (hash: %d) []", packet_count, i, max_packet_count, udp_packet_size, packet_hash);
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
    // free(c_buffer);
    //delete c_buffer;
}

int packet_sender_thread(void *arg) {
	StreamingEnvironment *se = (StreamingEnvironment*)arg;

    try
    {
        auto const address = boost::asio::ip::make_address(LISTENING_ADDRESS);
        auto const port = static_cast<unsigned short>(network_PORT);

        // The io_context is required for all I/O
        boost::asio::io_context ioc{1};
//        tcp::acceptor acceptor(ios, tcp::endpoint(tcp::v4(), port));


        // Création du service principal et du résolveur.
        boost::asio::io_service ios;

        // Création de l'acceptor avec le port d'écoute 7171 et une adresse quelconque de type IPv4 // (1)
        tcp::acceptor acceptor(ios, tcp::endpoint(tcp::v4(), port));

        std::string msg ("Bienvenue sur le serveur !"); // (2)
        // On attend la venue d'un client
        while (1)
        {
            // Création d'une socket
            tcp::socket socket(ios); // (3)
//            socket.set_option(tcp::no_delay(true));

            // On accepte la connexion
            acceptor.accept(socket); // (4)
            std::cout << "Client reçu ! " << std::endl;

            do_session(socket, se);

//            // On envoi un message de bienvenue
//            socket.send(boost::asio::buffer(msg)); // (5)
        }

//        tcp::socket sock(ioc, tcp::endpoint(tcp::v4(), port));
//        for (;;)
//        {
//            char data[BUFFER_SIZE];
//            tcp::endpoint sender_endpoint;
//            acceptor.accept(socket);
//            size_t length = sock.read_some(boost::asio::buffer(data, BUFFER_SIZE));
//            do_session(sock, sender_endpoint, se);
//        }
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
    std::vector<int8_t> bytes;
    std::vector<int> processed_packets;
    int expected_packet_count;
    int received_packet_count;
} map_packet_entry;


fifo_map<int, map_packet_entry*> map_of_incoming_buffers;

void flush_packets(StreamingEnvironment *se, fifo_map<int, map_packet_entry *> &m, int last_packet_number) {

    for(auto iter = m.cbegin(); iter != m.cend();) {
        int packet_number =  iter->first;
        map_packet_entry* map_entry = iter->second;

        std::chrono::system_clock::time_point t2 = std::chrono::system_clock::now();

        if (map_entry->expected_packet_count == map_entry->received_packet_count) {
            long hash = compute_quick_n_dirty_hash((char *) map_entry->bytes.data(), map_entry->total_size);

            packet_data *network_packet_data = new packet_data();
            network_packet_data->size = map_entry->total_size;
            network_packet_data->data = new uint8_t[map_entry->total_size - sizeof(packet_data)];
            std::chrono::system_clock::time_point t3 = std::chrono::system_clock::now();

            network_packet_data->size = map_entry->total_size - sizeof(packet_data);
            memcpy(network_packet_data->data, map_entry->bytes.data() + sizeof(packet_data), map_entry->total_size - sizeof(packet_data));

            std::chrono::system_clock::time_point t4 = std::chrono::system_clock::now();

            simple_queue_push(se->network_simulated_queue, network_packet_data);
            std::chrono::system_clock::time_point t5 = std::chrono::system_clock::now();

            map_packet_entry* entry = iter->second;
            delete entry;
        }
        
        m.erase(iter++);
    }
}

int packet_receiver_thread(void *arg) {
	StreamingEnvironment *se = (StreamingEnvironment*)arg;

    auto const host = SERVER_ADDRESS;
    auto const port = network_PORT;
    auto const text = "HelloWorld from a network client!";

    usleep(3 * 1000);

    // The io_service is required for all I/O
    boost::asio::io_service ios;

    // On veut se connecter sur la machine locale, port 7171
    tcp::endpoint endpoint(boost::asio::ip::address::from_string(host), port);

    // On crée une socket // (1)
    tcp::socket socket(ios);

    // Tentative de connexion, bloquante // (2)
    socket.connect(endpoint);
//    socket.set_option(tcp::no_delay(true));

    // Send the message
    std::chrono::system_clock::time_point t0a = std::chrono::system_clock::now();
    socket.send(boost::asio::buffer(std::string(text)));
    std::chrono::system_clock::time_point t0b = std::chrono::system_clock::now();
    float d0 = std::chrono::duration_cast<std::chrono::microseconds>(t0b - t0a).count() / 1000.0;
    log_info(" - d0 %f ms", d0);

    int8_t reply[BUFFER_SIZE];

    udp::endpoint sender_endpoint;
    int expected_packet_number = 0;
    auto buffer_reply = boost::asio::buffer(reply, BUFFER_SIZE);
    while (!se->finishing) {

        std::chrono::system_clock::time_point t1 = std::chrono::system_clock::now();
        size_t reply_length = s.receive_from(buffer_reply, sender_endpoint);

        int8_t* data = (int8_t*) buffer_reply.data();
        int8_t* payload_address = data + 5 * sizeof(int);
        long payload_size = reply_length - 5 * sizeof(int);
        int packet_index = ((int *) data)[0];
        int expected_packet_count = ((int *) data)[1];
        int packet_number = ((int *) data)[2];
        int packet_data_size = ((int *) data)[3];
        int offset = ((int *) data)[4];
        std::chrono::system_clock::time_point t2 = std::chrono::system_clock::now();

        if (map_of_incoming_buffers.find(packet_number) == map_of_incoming_buffers.end()) {
            map_packet_entry *new_entry = new map_packet_entry();
            new_entry->packet_number = packet_number;
            new_entry->total_size = packet_data_size;
            new_entry->copied_bytes = 0;
            new_entry->expected_packet_count = expected_packet_count;
            new_entry->received_packet_count = 0;
            map_of_incoming_buffers[packet_number] = new_entry;
        }

        std::chrono::system_clock::time_point t3 = std::chrono::system_clock::now();
        map_packet_entry *map_entry = map_of_incoming_buffers[packet_number];

        
        std::chrono::system_clock::time_point t4 = std::chrono::system_clock::now();
        std::vector<uint8_t> v(payload_address, payload_address + payload_size);
        map_entry->bytes.insert(map_entry->bytes.end(), v.begin(), v.end());
        v.erase(v.begin(), v.end());

        map_entry->processed_packets.insert(map_entry->processed_packets.end(), packet_index);

        map_entry->copied_bytes += payload_size;
        map_entry->received_packet_count += 1;

        long packet_hash = compute_quick_n_dirty_hash((char*) data, reply_length);
         log_info("[network] read sub packet [%d %d/%d]: %d bytes (hash: %d)", packet_number, packet_index, map_entry->expected_packet_count, reply_length, packet_hash);

        if (map_entry->received_packet_count == expected_packet_count) {
            if(packet_number > expected_packet_number) {
                // A packet may have been dropped!
                log_info("A packet may have been dropped...");
            }

            if (payload_size == 0) {
                continue;
            }

            expected_packet_number = packet_number + 1;

            flush_packets(se, map_of_incoming_buffers, packet_number);
        }
    }

    // If we get here then the connection is closed gracefully
	return 0;
}