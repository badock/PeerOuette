#include <SDL.h>
#include <SDL_thread.h>
#include <string>
#include <sstream>
#include <iostream>


#if defined(WIN32)
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>
#include <crtdbg.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <unistd.h>
#endif

#ifndef GAMECLIENTSDL_STREAMING_H
#define GAMECLIENTSDL_STREAMING_H

#include "src/codec/codec.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <stdlib.h>
}

#include <chrono>
#include "src/queue/queue.h"
#include "src/log/log.h"
#include <queue>

#if defined(WIN32)
void usleep(unsigned int usec);
#else
#include <unistd.h>
#endif

typedef struct _FrameData {
    AVFrame *pFrame;
    uint8_t *buffer;
    int numBytes;
    int id;
    int pitch;
    int stride;
    // debug field
    std::chrono::system_clock::time_point life_started_time_point;
    std::chrono::system_clock::time_point dxframe_acquired_time_point;
    std::chrono::system_clock::time_point dxframe_processed_time_point;
    std::chrono::system_clock::time_point avframe_produced_time_point;
    std::chrono::system_clock::time_point sdl_received_time_point;
    std::chrono::system_clock::time_point sdl_avframe_rescale_time_point;
    std::chrono::system_clock::time_point sdl_displayed_time_point;
} FrameData;


typedef struct _StreamingEnvironment {
	SDL_Thread *frame_extractor_thread;
	SDL_Thread *frame_encoder_thread;
	SDL_Thread *frame_decoder_thread;
	SDL_Thread *frame_output_thread;
	SDL_Thread *video_encode_thread;
	SDL_Thread *video_decode_thread;
	SDL_Thread *packet_sender_thread;
    SDL_Thread *asio_udp_listener;
	SDL_Thread *packet_receiver_thread;
#if defined(WIN32)
	SDL_Thread *gpu_frame_extractor_thread;
#endif
    SimpleQueue<packet_data*> network_simulated_queue;
	SimpleQueue<FrameData*> frame_extractor_pframe_pool;
	SimpleQueue<FrameData*> frame_sender_thread_queue;
//	SimpleQueue frame_receiver_thread_queue;
//	SimpleQueue *packet_sender_thread_queue;
    SimpleQueue<packet_data*> packet_sender_thread_queue;
	SimpleQueue<FrameData*> frame_output_thread_queue;
//    SimpleQueue incoming_asio_buffer_queue;
	AVPixelFormat format;

	AVCodecContext* frameExtractorDecodingContext;
	AVFormatContext* frameExtractorEncodingFormatContext;

	int width;
	int height;

	AVCodec* encoder;
	AVCodec* decoder;
	
	SDL_Window *screen;
	SDL_Renderer *renderer;
	int videoStream;
	int initialized;
	int network_initialized;
	int screen_is_initialized;
	int finishing;

	bool client_connected = false;
	bool is_all_in_one = false;
	bool is_client = false;
	bool is_server = false;

	std::string listen_address;
    std::string server_address;
//    int server_port;
} StreamingEnvironment;

void frame_data_reset_time_points(FrameData* frame_data);
void frame_data_debug(FrameData* frame_data);
FrameData* frame_data_create(StreamingEnvironment* se);
int frame_data_destroy(FrameData* frame_data);
#endif