#include "src/streaming/streaming.h"

#if WIN32
#include <ws2tcpip.h>
#endif

#include <stdlib.h>
#include <stdio.h>

int win_client_thread(void *arg);
int win_server_thread(void *arg);