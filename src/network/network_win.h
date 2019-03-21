#include "src/streaming/streaming.h"
#include "src/codec/codec.h"

#include <boost/beast/websocket.hpp>
#include <boost/beast/version.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <nlohmann/fifo_map.hpp>

//using tcp = boost::asio::ip::tcp; // from <boost/asio.hpp>
//namespace websocket = boost::beast::websocket; // from <beast/websocket.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using udp = boost::asio::ip::udp;
using tcp = boost::asio::ip::tcp;
using nlohmann::fifo_map;

typedef struct map_packet_entry {
    int packet_number;
    int total_size;
    int copied_bytes;
    std::vector<int8_t> bytes;
    std::vector<int> processed_packets;
    int expected_packet_count;
    int received_packet_count;
} map_packet_entry;

int packet_sender_thread(void *arg);
int asio_udp_listener(void *arg);
int packet_receiver_thread(void *arg);