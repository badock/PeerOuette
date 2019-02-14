#include "network_win.h"

#if WIN32
// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 900000
#define DEFAULT_PORT "9000"
#define SERVER_URL "192.168.1.23"
//#define SERVER_URL "127.0.0.1"
#endif

#define USE_NETWORK false

char* get_ffmpeg_error_msg(int error_code) {
	char myArray[AV_ERROR_MAX_STRING_SIZE] = { 0 }; // all elements 0
	char * msg = av_make_error_string(myArray, AV_ERROR_MAX_STRING_SIZE, error_code);
	int len = strlen(msg);
	char* result = (char*) malloc(len * sizeof(char));
	strcpy(result, msg);
	return result;
}

typedef struct serialized_packet_ {
	uint8_t* data;
	int size;
} serialized_packet;

int win_client_thread(void *arg) {
	StreamingEnvironment *se = (StreamingEnvironment*)arg;
    std::chrono::system_clock::time_point before = std::chrono::system_clock::now();

#if WIN32

	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	char *sendbuf = "this is a test";
	char recvbuf[DEFAULT_BUFLEN];
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	// Resolve the server address and port
	iResult = getaddrinfo(SERVER_URL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	// Send an initial buffer
	iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
	if (iResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	printf("Bytes Sent: %ld\n", iResult);
	//iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
#endif

	FrameData* frame_data = (FrameData*)simple_queue_pop(se->frame_extractor_pframe_pool);

	while (se->finishing != 1) {

		// Receive until the peer closes the connection
		AVPacket* pkt;

		if (USE_NETWORK) {
#if WIN32
			pkt = (AVPacket*) malloc(sizeof(AVPacket));
			av_init_packet(pkt);
			iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);

			pkt->data = (uint8_t*)recvbuf;
			pkt->size = iResult;
#endif
		}
		else {
			pkt = (AVPacket*) simple_queue_pop(se->network_simulated_queue);
		}
		int ret;

		if (pkt) {
			ret = avcodec_send_packet(se->pDecodingCtx, pkt);
			if (USE_NETWORK) {
#if WIN32
				free(pkt);
#endif
			}
			if (ret < 0) {
				continue;
			}
		}

		ret = avcodec_receive_frame(se->pDecodingCtx, frame_data->pFrame);
		if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
			continue;
		} else if (ret >= 0) {
            std::chrono::system_clock::time_point after = frame_data->sdl_displayed_time_point = std::chrono::system_clock::now();
            float frame_encode_duration = std::chrono::duration_cast<std::chrono::microseconds>(after - before).count() / 1000.0;
            log_info(" decoding duration: %f", frame_encode_duration);

            simple_queue_push(se->frame_output_thread_queue, frame_data);
			frame_data = (FrameData*)simple_queue_pop(se->frame_extractor_pframe_pool);
            before = std::chrono::system_clock::now();
		}
	}

#if WIN32
	// cleanup
	closesocket(ConnectSocket);
	WSACleanup();
#endif
	return 0;
}

int win_server_thread(void *arg) {
	StreamingEnvironment *se = (StreamingEnvironment*)arg;

#if WIN32
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(SERVER_URL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);
	struct sockaddr_in servaddr, cliaddr;
	int len;

	sockaddr_in client;
	len = sizeof(client);
	iResult = recvfrom(ListenSocket, recvbuf, recvbuflen, 0, (struct sockaddr *)&client, (socklen_t *)&len);
	int port = ntohs(client.sin_port);
	if (iResult != -1) {
		log_info("recv()'d %d bytes of data in buf\n", iResult);
	}
#endif

	//////////////////////////////////////////////////////
	// <custom> BADOCK: READS AVFRAME AND SENDS PACKETS
	//////////////////////////////////////////////////////
	se->network_initialized = 1;

	AVPacket *pkt;
	while (se->finishing != 1) {
		FrameData* frame_data = (FrameData*) simple_queue_pop(se->frame_sender_thread_queue);

        std::chrono::system_clock::time_point before = frame_data->sdl_displayed_time_point = std::chrono::system_clock::now();
		int ret = avcodec_send_frame(se->pEncodingCtx, frame_data->pFrame);
		if (ret < 0) {
			char myArray[AV_ERROR_MAX_STRING_SIZE] = { 0 }; // all elements 0
			char * msg = av_make_error_string(myArray, AV_ERROR_MAX_STRING_SIZE, ret);
			fprintf(stderr, "Error sending a frame for encoding %s\n", msg);
			continue;
		}
		while (ret >= 0) {
			pkt = av_packet_alloc();
			int retReceivePacket = avcodec_receive_packet(se->pEncodingCtx, pkt);
			if (retReceivePacket == AVERROR(EAGAIN) || retReceivePacket == AVERROR_EOF) {
				break;
			}
			else if (ret < 0) {
				fprintf(stderr, "Error during encoding\n");
				exit(1);
			}

			int size = pkt->size;
			//printf("Write packet (size=%d)\n", size);

			if (USE_NETWORK) {
#if WIN32
				//iSendResult = send(ClientSocket, (char*) pkt->data, pkt->size, 0);

				iSendResult = sendto(ListenSocket, (char*)pkt->data, pkt->size,
					0, (const struct sockaddr *) &client,
					len);
#endif
			}
			else {
				simple_queue_push(se->network_simulated_queue, pkt);
			}
		}
        std::chrono::system_clock::time_point after = frame_data->sdl_displayed_time_point = std::chrono::system_clock::now();
        float frame_encode_duration = std::chrono::duration_cast<std::chrono::microseconds>(after - before).count() / 1000.0;

        log_info(" encoding duration: %f", frame_encode_duration);

        simple_queue_push(se->frame_extractor_pframe_pool, frame_data);
//		simple_queue_push(se->frame_output_thread_queue, frame_data);
	}

	//////////////////////////////////////////////////////
	// </custom>
	//////////////////////////////////////////////////////

#if WIN32
	// shutdown the connection since we're done
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(ClientSocket);
	WSACleanup();
#endif
	return 0;
}