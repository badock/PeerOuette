#include "src/streaming/streaming.h"

// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

#include "src/network/network_win.h"
#include "src/codec/codec.h"
#if defined(WIN32)
#include "src/dxcapture/capture.h"
#include "src/texture_converter/TextureConverter.h"
#else
#include <unistd.h>
#endif

#undef main

int SDL_WINDOW_WIDTH = 1280;
int SDL_WINDOW_HEIGHT = 720;
int CAPTURE_WINDOW_WIDTH = 1920;
int CAPTURE_WINDOW_HEIGHT = 1080;
int BITRATE = CAPTURE_WINDOW_WIDTH * CAPTURE_WINDOW_HEIGHT * 3;
int FRAMERATE = 60;
int FRAME_POOL_SIZE = 100;
char* VIDEO_FILE_PATH = "misc/rogue.mp4";

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

    int frameFinished;
	ID3D11Texture2D* CopyBuffer = nullptr;
    int64_t framecount = 0;
    while(1) {
		FrameData* ffmpeg_frame_data = (FrameData *) simple_queue_pop(se->frame_extractor_pframe_pool);
		frame_data_reset_time_points(ffmpeg_frame_data);
	    
		int capture_result = capture_frame(&cc, d3d_frame_data, ffmpeg_frame_data);
		ffmpeg_frame_data->dxframe_processed_time_point = std::chrono::system_clock::now();

		int result = get_pixels_yuv420p(&cc, ffmpeg_frame_data);
        
		ffmpeg_frame_data->pFrame->format = AV_PIX_FMT_YUV420P;
		ffmpeg_frame_data->avframe_produced_time_point = std::chrono::system_clock::now();

		int frame_release_result = done_with_frame(&cc);

		if (true) {
			simple_queue_push(se->frame_sender_thread_queue, ffmpeg_frame_data);
		}
		else {
			simple_queue_push(se->frame_output_thread_queue, ffmpeg_frame_data);
		}

        // usleep(16.666 * 1000);
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

    // [SDL] Free the YUV image
    log_info("Free the YUV image");
    free(yPlane);
    free(uPlane);
    free(vPlane);

    return 0;
}

int frame_extractor_thread(void *arg) {
	StreamingEnvironment *se = (StreamingEnvironment*)arg;

	// Initialize streaming environment and threads
    AVFormatContext *pFormatCtx;
    AVCodecContext* frameExtractorEncodingContext;
	//AVPacket packet;

	// Initialize streaming environment and threads
	se->frameExtractorEncodingContext = NULL;

	// [FFMPEG] Registering file formats and codecs
	log_info("Registering file formats and codecs");
	av_register_all();
	//    AVFormatContext *pFormatCtx = NULL;

		// [FFMPEG] Open video file
	log_info("Open video file: %s", VIDEO_FILE_PATH);
	int result = avformat_open_input(&se->frameExtractorEncodingFormatContext, VIDEO_FILE_PATH, NULL, NULL);
	if (result != 0) {
		char myArray[AV_ERROR_MAX_STRING_SIZE] = { 0 }; // all elements 0
		char * plouf = av_make_error_string(myArray, AV_ERROR_MAX_STRING_SIZE, result);
		fprintf(stderr, "Error occurred: %s\n", plouf);
		log_error("Could not open video file: %s", VIDEO_FILE_PATH);
		return -1;
	}

	// [FFMPEG] Retrieve stream information
	log_info("Retrieve stream information");
	if (avformat_find_stream_info(se->frameExtractorEncodingFormatContext, NULL) < 0) {
		log_error("Couldn't find stream information");
		return -1;
	}

	// [FFMPEG] Dump information about file onto standard error
	log_info("Dump information about file");
	av_dump_format(se->frameExtractorEncodingFormatContext, 0, VIDEO_FILE_PATH, 0);

	int i;
	AVCodecContext *frameExtractorEncodingContextOrig = NULL;
	se->frameExtractorEncodingContext = NULL;

	// [FFMPEG] Find the first video stream
	log_info("Find the first video stream");
	se->videoStream = -1;
	for (i = 0; i < se->frameExtractorEncodingFormatContext->nb_streams; i++)
		if (se->frameExtractorEncodingFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			se->videoStream = i;
			break;
		}
	if (se->videoStream == -1) {
		log_error("Didn't find a video stream");
		return -1;
	}

	// [FFMPEG] Get a pointer to the codec context for the video stream
	se->frameExtractorEncodingContext = se->frameExtractorEncodingFormatContext->streams[se->videoStream]->codec;

	AVCodec *pCodec = NULL;

	// [FFMPEG] Find the decoder for the video stream
	log_info("Find the decoder for the video stream");

	pCodec = avcodec_find_decoder(se->frameExtractorEncodingContext->codec_id);
	if (pCodec == NULL) {
		log_error("Unsupported codec!\n");
		return -1; // Codec not found
	}

	// Open codec
	log_info("Open codec");
	if (avcodec_open2(se->frameExtractorEncodingContext, pCodec, NULL) < 0) {
		log_error("Could not open codec");
		return -1;
	}


	while (!se->initialized) {
		usleep(30 * 1000);
	}

	// [FFMPEG] Reading frames
	log_info("Reading frames");
	SDL_Event event;
	AVPacket packet;
	i = 0;

	while (!se->screen_is_initialized) {
		usleep(30 * 1000);
	}

	int frameFinished;
	while (av_read_frame(se->frameExtractorEncodingFormatContext, &packet) >= 0) {
		// Is this a packet from the video stream?
		if (packet.stream_index == se->videoStream) {
			// [FFMPEG] Allocate video frame
//			log_info("frame_extractor_pframe_pool: %i elements in queue", simple_queue_length(se->frame_extractor_pframe_pool));
			FrameData* frame_data = (FrameData *)simple_queue_pop(se->frame_extractor_pframe_pool);
//			log_info("WRITE ====> %i (%i)", frame_data->id, i);

			// Decode video frame
			avcodec_decode_video2(se->frameExtractorEncodingContext
	, frame_data->pFrame, &frameFinished, &packet);

			// Did we get a video frame?
			if (frameFinished) {
				// Push frame to the output_video thread
				AVFrame* old_pframe = frame_data->pFrame;
				frame_data->pFrame = av_frame_clone(frame_data->pFrame);
				simple_queue_push(se->frame_sender_thread_queue, frame_data);

				//simple_queue_push(se->frame_output_thread_queue, frame_data);
				av_free(old_pframe);
				i++;

				// usleep(16.666 * 1000);
			}
		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
	}

	return 0;
}

void my_log_callback(void *ptr, int level, const char *fmt, va_list vargs)
{
	vprintf(fmt, vargs);
}

int main(int argc, char* argv[]){
    log_info("Simple GameClient has started");


	// [FFMPEG] Registering file formats and codecs
	log_info("Registering file formats and codecs");
	av_register_all();

    // av_log_set_callback(my_log_callback);
    // av_log_set_level(AV_LOG_VERBOSE);

    // Initialize streaming environment and threads
    log_info("Initializing streaming environment");
    StreamingEnvironment *se;
    se = (StreamingEnvironment*) av_mallocz(sizeof(StreamingEnvironment));
    se->frame_extractor_pframe_pool = simple_queue_create();
    se->frame_output_thread_queue = simple_queue_create();
	se->frame_sender_thread_queue = simple_queue_create();
	se->frame_receiver_thread_queue = simple_queue_create();
    se->packet_sender_thread_queue = simple_queue_create();
    se->network_simulated_queue = simple_queue_create();

	se->frame_output_thread = SDL_CreateThread(frame_output_thread, "frame_output_thread", se);
    // se->frame_extractor_thread = SDL_CreateThread(frame_extractor_thread, "frame_extractor_thread", se);
    #if defined(WIN32)
    se->gpu_frame_extractor_thread = SDL_CreateThread(gpu_frame_extractor_thread, "gpu_frame_extractor_thread", se);
    #endif

 	se->frame_receiver_thread = SDL_CreateThread(video_encode_thread, "frame_receiver_thread", se);
	se->frame_sender_thread = SDL_CreateThread(video_decode_thread, "frame_sender_thread", se);

    se->packet_receiver_thread = SDL_CreateThread(packet_receiver_thread, "packet_receiver_thread", se);
    se->packet_sender_thread = SDL_CreateThread(packet_sender_thread, "packet_sender_thread", se);

    se->finishing = 0;
    se->initialized = 0;
	se->network_initialized = 0;
    se->screen_is_initialized = 0;
    se->width = CAPTURE_WINDOW_WIDTH;
    se->height = CAPTURE_WINDOW_HEIGHT;
	se->format = AV_PIX_FMT_YUV420P;

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
	for (int i = 0; i < FRAME_POOL_SIZE; i++) {
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
        // Prevent high CPU usage
        SDL_Delay(1);
    }

    // [FFMPEG] Free the RGB image
    while (! simple_queue_is_empty(se->frame_extractor_pframe_pool)) {
        FrameData* frame_data = (FrameData *) simple_queue_pop(se->frame_extractor_pframe_pool);
        frame_data_destroy(frame_data);
    }

    log_info("Simple GameClient is exiting");
    
	return 0;
}
