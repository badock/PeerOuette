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
	AVFrame *pFrameRGB;
	uint8_t *buffer;
	int id;
} FrameData;
#endif