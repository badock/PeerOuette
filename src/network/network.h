#include "src/streaming/streaming.h"
#include "src/codec/codec.h"
#include <iostream>
#include <thread>
#include <cstdlib>
#include <iostream>
#include <string>

int packet_sender_thread(void *arg);
int packet_receiver_thread(void *arg);