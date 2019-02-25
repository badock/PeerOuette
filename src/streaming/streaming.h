#include <SDL.h>
#include <SDL_thread.h>

#if defined(WIN32)
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>
#include <Windows.h>
#else
#include <unistd.h>
#endif

#ifndef GAMECLIENTSDL_STREAMING_H
#define GAMECLIENTSDL_STREAMING_H

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

typedef struct _StreamingEnvironment {
	SDL_Thread *frame_extractor_thread;
	SDL_Thread *frame_encoder_thread;
	SDL_Thread *frame_decoder_thread;
	SDL_Thread *frame_output_thread;
	SDL_Thread *frame_receiver_thread;
	SDL_Thread *frame_sender_thread;
	SDL_Thread *packet_receiver_thread;
	SDL_Thread *packet_sender_thread;
#if defined(WIN32)
	SDL_Thread *gpu_frame_extractor_thread;
#endif
    SimpleQueue *network_simulated_queue;
	SimpleQueue *frame_extractor_pframe_pool;
	SimpleQueue *frame_sender_thread_queue;
	SimpleQueue *frame_receiver_thread_queue;
	SimpleQueue *packet_sender_thread_queue;
	SimpleQueue *frame_output_thread_queue;
	AVPixelFormat format;

	AVCodecContext* frameExtractorEncodingContext;
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
} StreamingEnvironment;

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

void frame_data_reset_time_points(FrameData* frame_data);
void frame_data_debug(FrameData* frame_data);
FrameData* frame_data_create(StreamingEnvironment* se);
int frame_data_destroy(FrameData* frame_data);
#endif