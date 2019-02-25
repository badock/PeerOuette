#include "src/streaming/streaming.h"
#include "src/codec/codec.h"

#include <boost/beast/websocket.hpp>
#include <boost/beast/version.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

//using tcp = boost::asio::ip::tcp; // from <boost/asio.hpp>
//namespace websocket = boost::beast::websocket; // from <beast/websocket.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

int packet_sender_thread(void *arg);
int packet_receiver_thread(void *arg);