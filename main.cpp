#include "src/streaming/streaming.h"

// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

#include "src/network/network_win.h"
#include "src/codec/codec.h"
#if defined(WIN32)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <winsock2.h>
#include <Windows.h>
#include "src/dxcapture/capture.h"
#include "src/texture_converter/TextureConverter.h"
void usleep(unsigned int usec)
{
	Sleep(usec / 1000);
}
#else
#include <unistd.h>
#endif

#undef main

int SDL_WINDOW_WIDTH = 1280;
int SDL_WINDOW_HEIGHT = 720;
int CAPTURE_WINDOW_WIDTH = 1920;
int CAPTURE_WINDOW_HEIGHT = 816;
int BITRATE = CAPTURE_WINDOW_WIDTH * CAPTURE_WINDOW_HEIGHT * 3;
int FRAMERATE = 60;
char* VIDEO_FILE_PATH = "misc/rogue.mp4";
//#define CODEC_ID AV_CODEC_ID_MPEG2VIDEO
//#define CODEC_ID AV_CODEC_ID_MPEG4
#define CODEC_ID AV_CODEC_ID_H264
//#define CODEC_ID AV_CODEC_ID_VP9

StreamingEnvironment *global_streaming_environment;


#if defined(WIN32)
int gpu_frame_extractor_thread(void *arg) {
    StreamingEnvironment *se = (StreamingEnvironment*) arg;

    while(se->initialized != 1) {
        usleep(50 * 1000);
    }

    int output = 0;
    CaptureContext cc;
	cc.m_MetaDataBuffer = (BYTE*) malloc(sizeof(BYTE) * 16);
	cc.m_MetaDataSize = 16;
	cc.capture_mode = YUV420P;

    init_directx(&cc);
    init_capture(&cc);
	init_video_mode(&cc);
	
	D3D_FRAME_DATA* d3d_frame_data = (D3D_FRAME_DATA*) malloc(sizeof(D3D_FRAME_DATA));
	//FrameData* ffmpeg_frame_data = frame_data_create(se);

    int frameFinished;
	ID3D11Texture2D* CopyBuffer = nullptr;
    while(1) {
		FrameData* ffmpeg_frame_data = (FrameData *) simple_queue_pop(se->frame_extractor_pframe_pool);
		frame_data_reset_time_points(ffmpeg_frame_data);
	    
		int capture_result = capture_frame(&cc, d3d_frame_data, ffmpeg_frame_data);
		ffmpeg_frame_data->dxframe_processed_time_point = std::chrono::system_clock::now();

		int result;
		////result = get_pixels(&cc, ffmpeg_frame_data);
		result = get_pixels_yuv420p(&cc, ffmpeg_frame_data);
		//// set metadata
		//ffmpeg_frame_data->pFrame->pict_type = AV_PICTURE_TYPE_I;
		////ffmpeg_frame_data->pFrame->pts = 1080*1920;
		//ffmpeg_frame_data->pFrame->pts = 0;
		//ffmpeg_frame_data->pFrame->pkt_pts = 0;
		//ffmpeg_frame_data->pFrame->pkt_dts = 0;
		//ffmpeg_frame_data->pFrame->sample_aspect_ratio.num = 1;
		//ffmpeg_frame_data->pFrame->color_range = AVCOL_RANGE_MPEG;

		//// Push frame to the output_video thread
		//AVFrame* old_pframe = ffmpeg_frame_data->pFrame;
		//ffmpeg_frame_data->pFrame = av_frame_alloc();
		//int buffer_size = avpicture_get_size(AV_PIX_FMT_YUV420P, CAPTURE_WINDOW_WIDTH, CAPTURE_WINDOW_WIDTH);
		//unsigned char* pic_buffer_in = (unsigned char*) malloc(buffer_size * sizeof(unsigned char*));
		//memcpy(pic_buffer_in, old_pframe->data, buffer_size);
		
		//ffmpeg_frame_data->pFrame = av_frame_clone(frame_data->pFrame);
		//simple_queue_push(se->frame_sender_thread_queue, frame_data);


		ffmpeg_frame_data->avframe_produced_time_point = std::chrono::system_clock::now();


		int frame_release_result = done_with_frame(&cc);

		if (se->network_initialized) {
			simple_queue_push(se->frame_sender_thread_queue, ffmpeg_frame_data);
		}
		else {
			simple_queue_push(se->frame_output_thread_queue, ffmpeg_frame_data);
		}	
    }

    return 0;
}
#endif

int frame_output_thread(void *arg) {
    StreamingEnvironment *se = (StreamingEnvironment*) arg;

    while(se->initialized != 1) {
        usleep(50 * 1000);
    }

    // [SDL] Create a YUV overlay
    log_info("Creating a YUV overlay");
    SDL_Texture *texture;
    struct SwsContext *sws_ctx = NULL;

    texture = SDL_CreateTexture(
            se->renderer,
            SDL_PIXELFORMAT_YV12,
            SDL_TEXTUREACCESS_STREAMING,
            se->width,
            se->height
    );
    if (!texture) {
        fprintf(stderr, "SDL: could not create texture - exiting\n");
        exit(1);
    }

    // initialize SWS context for software scaling
    sws_ctx = sws_getContext(se->width,
                             se->height,
                             se->format,
                             se->width,
                             se->height,
                             AV_PIX_FMT_YUV420P,
                             SWS_BILINEAR,
                             NULL,
                             NULL,
                             NULL);


    // [SDL] set up YV12 pixel array (12 bits per pixel)
    Uint8 *yPlane, *uPlane, *vPlane;
    size_t yPlaneSz, uvPlaneSz;
    int uvPitch;
    yPlaneSz = se->width * se->height;
    uvPlaneSz = se->width * se->height / 4;
    yPlane = (Uint8*)malloc(yPlaneSz);
    uPlane = (Uint8*)malloc(uvPlaneSz);
    vPlane = (Uint8*)malloc(uvPlaneSz);
    if (!yPlane || !uPlane || !vPlane) {
        fprintf(stderr, "Could not allocate pixel buffers - exiting\n");
        exit(1);
    }

    uvPitch = se->width / 2;

    // Screen is ready
    log_info("[SDL] screen is ready");
    se->screen_is_initialized = 1;

    int i = 0;
    while(se->finishing != 1) {
        //log_info("frame_output_thread: %i elements in queue", simple_queue_length(se->frame_output_thread_queue));
        FrameData* frame_data = (FrameData*) simple_queue_pop(se->frame_output_thread_queue);
		frame_data->sdl_received_time_point = std::chrono::system_clock::now();

        // [SDL] Create an AV Picture
        AVPicture pict;
        pict.data[0] = yPlane;
        pict.data[1] = uPlane;
        pict.data[2] = vPlane;
        pict.linesize[0] = CAPTURE_WINDOW_WIDTH;
        pict.linesize[1] = uvPitch;
        pict.linesize[2] = uvPitch;

        // Convert the image into YUV format that SDL uses
        sws_scale(sws_ctx,
			      (uint8_t const * const *) frame_data->pFrame->data,
                  frame_data->pFrame->linesize,
			      0,
				  CAPTURE_WINDOW_HEIGHT,
			      pict.data,
                  pict.linesize);
		frame_data->sdl_avframe_rescale_time_point = std::chrono::system_clock::now();

        // [SDL] update SDL overlay
        SDL_UpdateYUVTexture(
                texture,
                NULL,
                yPlane,
				CAPTURE_WINDOW_WIDTH,
                uPlane,
                uvPitch,
                vPlane,
                uvPitch
        );
        SDL_RenderClear(se->renderer);
        SDL_RenderCopy(se->renderer, texture, NULL, NULL);
        SDL_RenderPresent(se->renderer);
		frame_data->sdl_displayed_time_point = std::chrono::system_clock::now();
		
        i++;

		frame_data_debug(frame_data);
		simple_queue_push(se->frame_extractor_pframe_pool, frame_data);

		while (simple_queue_length(se->frame_output_thread_queue) > 0) {
			FrameData* frame_data = (FrameData*)simple_queue_pop(se->frame_output_thread_queue);
			simple_queue_push(se->frame_extractor_pframe_pool, frame_data);
		}
    }

    // [SDL] Cleaning SDL data
    log_info("Cleaning SDL data");
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(se->renderer);
    SDL_DestroyWindow(se->screen);

    // [SDL] Free the YUV image
    log_info("Free the YUV image");
    free(yPlane);
    free(uPlane);
    free(vPlane);

    return 0;
}

void my_log_callback(void *ptr, int level, const char *fmt, va_list vargs)
{
//    printf("[ffmpeg] %s\n", fmt);
}

int frame_extractor_thread(void *arg) {
	StreamingEnvironment *se = (StreamingEnvironment*)arg;

	// Initialize streaming environment and threads
    AVFormatContext *pFormatCtx;
    AVCodecContext* pCodecCtx;
    AVPacket packet;

	// [FFMPEG] Registering file formats and codecs
	log_info("Registering file formats and codecs");
	av_register_all();

    av_log_set_callback(my_log_callback);
    av_log_set_level(AV_LOG_VERBOSE);

    // [FFMPEG] Registering file formats and codecs
    log_info("Registering file formats and codecs");
    av_register_all();

    // [FFMPEG] Open video file
	log_info("Open video file: %s", VIDEO_FILE_PATH);
	int result = avformat_open_input(&pFormatCtx, VIDEO_FILE_PATH, NULL, NULL);
	if (result != 0) {
		char myArray[AV_ERROR_MAX_STRING_SIZE] = { 0 }; // all elements 0
		char * plouf = av_make_error_string(myArray, AV_ERROR_MAX_STRING_SIZE, result);
		fprintf(stderr, "Error occurred: %s\n", plouf);
		log_error("Could not open video file: %s", VIDEO_FILE_PATH);
		return -1;
	}

	// [FFMPEG] Retrieve stream information
	log_info("Retrieve stream information");
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		log_error("Couldn't find stream information");
		return -1;
	}

	// [FFMPEG] Dump information about file onto standard error
	log_info("Dump information about file");
	av_dump_format(pFormatCtx, 0, VIDEO_FILE_PATH, 0);

	int i;

	// [FFMPEG] Find the first video stream
	log_info("Find the first video stream");
	se->videoStream = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			se->videoStream = i;
			break;
		}
	if (se->videoStream == -1) {
		log_error("Didn't find a video stream");
		return -1;
	}

	// [FFMPEG] Get a pointer to the codec context for the video stream
	pCodecCtx = pFormatCtx->streams[se->videoStream]->codec;

	AVCodec *pCodec = NULL;

	// [FFMPEG] Find the decoder for the video stream
	log_info("Find the decoder for the video stream");

	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		log_error("Unsupported codec!\n");
		return -1; // Codec not found
	}

	// Open codec
	log_info("Open codec");
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		log_error("Could not open codec");
		return -1;
	}


	while (!se->initialized) {
		usleep(30 * 1000);
	}

	// [FFMPEG] Reading frames
	log_info("Reading frames");
	SDL_Event event;
	i = 0;

	while (!se->screen_is_initialized) {
		usleep(30 * 1000);
	}

	int frameFinished;
	while (av_read_frame(pFormatCtx, &packet) >= 0) {
		// Is this a packet from the video stream?
		if (packet.stream_index == se->videoStream) {
			FrameData* frame_data = (FrameData *)simple_queue_pop(se->frame_extractor_pframe_pool);

			// Decode video frame
			avcodec_decode_video2(pCodecCtx, frame_data->pFrame, &frameFinished, &packet);

			// Did we get a video frame?
			if (frameFinished) {
				// Push frame to the output_video thread
				AVFrame* old_pframe = frame_data->pFrame;
				frame_data->pFrame = av_frame_clone(frame_data->pFrame);
				simple_queue_push(se->frame_sender_thread_queue, frame_data);

				//simple_queue_push(se->frame_output_thread_queue, frame_data);
				av_free(old_pframe);
				i++;

				usleep(16.666 * 1000);
			}
		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
	}

	return 0;
}

int main(int argc, char* argv[]){
    log_info("Simple GameClient has started");

    // Initialize streaming environment and threads
    log_info("Initializing streaming environment");
    StreamingEnvironment *se;
    se = (StreamingEnvironment*) av_mallocz(sizeof(StreamingEnvironment));
    se->frame_extractor_pframe_pool = simple_queue_create();
    se->frame_output_thread_queue = simple_queue_create();
	se->frame_sender_thread_queue = simple_queue_create();
	se->frame_receiver_thread_queue = simple_queue_create();
	se->network_simulated_queue = simple_queue_create();
	se->frame_output_thread = SDL_CreateThread(frame_output_thread, "frame_output_thread", se);
    se->frame_extractor_thread = SDL_CreateThread(frame_extractor_thread, "frame_extractor_thread", se);
    #if defined(WIN32)
    se->gpu_frame_extractor_thread = SDL_CreateThread(gpu_frame_extractor_thread, "gpu_frame_extractor_thread", se);
    #endif
//    se->frame_receiver_thread = SDL_CreateThread(win_client_thread, "frame_receiver_thread", se);
//    se->frame_sender_thread = SDL_CreateThread(win_server_thread, "frame_sender_thread", se);

    se->frame_receiver_thread = SDL_CreateThread(video_encode_thread, "frame_receiver_thread", se);
    se->frame_sender_thread = SDL_CreateThread(video_decode_thread, "frame_sender_thread", se);

    se->finishing = 0;
    se->initialized = 0;
	se->network_initialized = 0;
    se->screen_is_initialized = 0;
    se->width = CAPTURE_WINDOW_WIDTH;
    se->height = CAPTURE_WINDOW_HEIGHT;
    se->format = AV_PIX_FMT_YUV420P;

	//////////////////////////////////////////////////////////////

    // [SDL] Initializing SDL library
    log_info("Initializing SDL library");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        exit(1);
    }

    // [SDL] Creating a display
    log_info("Creating a SDL display");
    Uint32 screen_flags = SDL_WINDOW_OPENGL;
    if (0) {
        screen_flags = SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL;
    }
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
    SDL_Window *screen = screen = SDL_CreateWindow(
            "FFmpeg Tutorial",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOW_WIDTH,
			SDL_WINDOW_HEIGHT,
            screen_flags
    );
    SDL_ShowWindow(screen);
    se->screen = screen;

    if (!se->screen) {
        fprintf(stderr, "SDL: could not create window - exiting\n");
        exit(1);
    }

    se->renderer = SDL_CreateRenderer(se->screen, -1, 0);
    if (!se->renderer) {
        fprintf(stderr, "SDL: could not create renderer - exiting\n");
        exit(1);
    }

    // Application is ready to read frame and display frames
    se->initialized = 1;

	// [FFMPEG] Initialize frame pool
	for (int i = 0; i < 30; i++) {
		FrameData* frame_data = frame_data_create(se);
		frame_data->id = i;
		simple_queue_push(se->frame_extractor_pframe_pool, frame_data);
	}

    SDL_Event event;

    while(! se->screen_is_initialized) {
        usleep(30 * 1000);
    }

    while(!se->finishing) {
        // [SDL] handle events
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    se->finishing = 1;
                    SDL_Quit();
                    break;
                default:
                    break;
            }
        }
    }

    // [FFMPEG] Free the RGB images
    while (! simple_queue_is_empty(se->frame_extractor_pframe_pool)) {
        FrameData* frame_data = (FrameData *) simple_queue_pop(se->frame_extractor_pframe_pool);
        frame_data_destroy(frame_data);
    }

    log_info("Simple GameClient is exiting");

    
	return 0;
}
