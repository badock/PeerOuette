#include <SDL.h>
#include <SDL_thread.h>

#ifndef GAMECLIENTSDL_STREAMING_H
#define GAMECLIENTSDL_STREAMING_H

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <stdlib.h>
}

#include <chrono>
#include "src/queue/queue.h"

typedef struct _StreamingEnvironment {
	SDL_Thread *frame_extractor_thread;
	SDL_Thread *frame_encoder_thread;
	SDL_Thread *frame_decoder_thread;
	SDL_Thread *frame_output_thread;
#if defined(WIN32)
	SDL_Thread *gpu_frame_extractor_thread;
#endif
	SimpleQueue * frame_extractor_pframe_pool;
	SimpleQueue *frame_output_thread_queue;
	AVCodecContext* pCodecCtx;
	AVFormatContext *pFormatCtx;
	SDL_Window *screen;
	SDL_Renderer *renderer;
	int videoStream;
	int initialized;
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
	std::chrono::steady_clock::time_point life_started_time_point;
	std::chrono::steady_clock::time_point dxframe_acquired_time_point;
	std::chrono::steady_clock::time_point avframe_produced_time_point;
	std::chrono::steady_clock::time_point sdl_received_time_point;
	std::chrono::steady_clock::time_point sdl_avframe_rescale_time_point;
	std::chrono::steady_clock::time_point sdl_displayed_time_point;
} FrameData;
#endif