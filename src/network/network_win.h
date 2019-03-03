#include "src/streaming/streaming.h"
#include "src/codec/codec.h"

#include <boost/beast/websocket.hpp>
#include <boost/beast/version.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

//using tcp = boost::asio::ip::tcp; // from <boost/asio.hpp>
//namespace websocket = boost::beast::websocket; // from <beast/websocket.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using udp = boost::asio::ip::udp;
int packet_sender_thread(void *arg);
int packet_receiver_thread(void *arg);