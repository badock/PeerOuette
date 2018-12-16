#include "src/streaming/streaming.h"

#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

int win_client_thread(void *arg);
int win_server_thread(void *arg);