#include "src/log/log.h"

#include "src/streaming/streaming.h"

// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

#if defined(WIN32)
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

StreamingEnvironment *global_streaming_environment;

void frame_data_reset_time_points(FrameData* frame_data) {
	frame_data->life_started_time_point = std::chrono::system_clock::now();
	frame_data->dxframe_acquired_time_point = std::chrono::system_clock::now();
	frame_data->dxframe_processed_time_point = std::chrono::system_clock::now();
	frame_data->avframe_produced_time_point = std::chrono::system_clock::now();
	frame_data->sdl_received_time_point = std::chrono::system_clock::now();
	frame_data->sdl_avframe_rescale_time_point = std::chrono::system_clock::now();
	frame_data->sdl_displayed_time_point = std::chrono::system_clock::now();
}

void frame_data_debug(FrameData* frame_data) {
	//log_info("####### Debug a frame #######");
	std::chrono::steady_clock::time_point start;
	std::chrono::steady_clock::time_point end;

	float dxframe_acquired_duration = std::chrono::duration_cast<std::chrono::microseconds>(frame_data->dxframe_acquired_time_point - frame_data->life_started_time_point).count() / 1000.0;
	float dxframe_processed_duration = std::chrono::duration_cast<std::chrono::microseconds>(frame_data->dxframe_processed_time_point - frame_data->dxframe_acquired_time_point).count() / 1000.0;
	float avframe_produced_duration = std::chrono::duration_cast<std::chrono::microseconds>(frame_data->avframe_produced_time_point - frame_data->dxframe_processed_time_point).count() / 1000.0;
	float sdl_received_duration = std::chrono::duration_cast<std::chrono::microseconds>(frame_data->sdl_received_time_point - frame_data->avframe_produced_time_point).count() / 1000.0;
	float sdl_avframe_duration = std::chrono::duration_cast<std::chrono::microseconds>(frame_data->sdl_avframe_rescale_time_point - frame_data->sdl_received_time_point).count() / 1000.0;
	float sdl_displayed_duration = std::chrono::duration_cast<std::chrono::microseconds>(frame_data->sdl_displayed_time_point - frame_data->sdl_avframe_rescale_time_point).count() / 1000.0;
	float total_duration = std::chrono::duration_cast<std::chrono::microseconds>(frame_data->sdl_displayed_time_point - frame_data->life_started_time_point).count() / 1000.0;
	float frame_delay = std::chrono::duration_cast<std::chrono::microseconds>(frame_data->sdl_displayed_time_point - frame_data->dxframe_acquired_time_point).count() / 1000.0;

	//log_info("# dxframe_acquired_duration = %f ms", dxframe_acquired_duration);
	//log_info("# avframe_produced_duration = %f ms", avframe_produced_duration);
	//log_info("# sdl_received_duration = %f ms", sdl_received_duration);
	//log_info("# sdl_avframe_duration = %f ms", sdl_avframe_duration);
	//log_info("# sdl_displayed_duration = %f ms", sdl_displayed_duration);
	//log_info("# total_duration = %f ms", (1.0 * total_duration));
	log_info("# frame_delay = %f ms", (1.0 * frame_delay));
	//log_info("##############");
}

FrameData* frame_data_create(StreamingEnvironment* se) {
    FrameData* frame_data = (FrameData*) malloc(sizeof(FrameData));

    // [FFMPEG] Allocate an AVFrame structure
    frame_data->pFrame = av_frame_alloc();
    if(frame_data->pFrame ==NULL)
        return NULL;

    // [FFMPEG] Determine required buffer size and allocate buffer
    int numBytes;
	int width = 1920;
	int height = 1080;
	//int width = se->pCodecCtx->width;
	//int height = se->pCodecCtx->height;

	numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P,
		width,
		height);
    frame_data->buffer=(uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
	frame_data->numBytes = numBytes;

    // [FFMPEG] Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
    // of AVPicture
	avpicture_fill((AVPicture *)frame_data->pFrame,
		           frame_data->buffer,
		           AV_PIX_FMT_YUV420P,
		           width,
		           height);

    frame_data->pFrame = av_frame_alloc();

	// Initialize the pFrame	
	frame_data->pFrame->width = width;
	frame_data->pFrame->height = height;

	frame_data->pFrame->linesize[0] = width;
	frame_data->pFrame->linesize[1] = width / 2;
	frame_data->pFrame->linesize[2] = width / 2;

	frame_data->pFrame->data[0] = frame_data->buffer;
	frame_data->pFrame->data[1] = frame_data->buffer + frame_data->pFrame->linesize[0] * height;
	frame_data->pFrame->data[2] = frame_data->buffer + frame_data->pFrame->linesize[0] * height + frame_data->pFrame->linesize[1] * height / 2;

    return frame_data;
}

int frame_data_destroy(FrameData* frame_data) {
    av_frame_free(&frame_data->pFrame);
    av_free(frame_data->buffer);
    free(frame_data);

	return 0;
}

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
		ffmpeg_frame_data->avframe_produced_time_point = std::chrono::system_clock::now();

		int frame_release_result = done_with_frame(&cc);

		simple_queue_push(se->frame_output_thread_queue, ffmpeg_frame_data);		
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
            se->pDecodingCtx->width,
            se->pDecodingCtx->height
    );
    if (!texture) {
        fprintf(stderr, "SDL: could not create texture - exiting\n");
        exit(1);
    }

    // initialize SWS context for software scaling
    sws_ctx = sws_getContext(se->pDecodingCtx->width,
                             se->pDecodingCtx->height,
                             se->pDecodingCtx->pix_fmt,
                             se->pDecodingCtx->width,
                             se->pDecodingCtx->height,
                             AV_PIX_FMT_YUV420P,
                             SWS_BILINEAR,
                             NULL,
                             NULL,
                             NULL);


    // [SDL] set up YV12 pixel array (12 bits per pixel)
    Uint8 *yPlane, *uPlane, *vPlane;
    size_t yPlaneSz, uvPlaneSz;
    int uvPitch;
    yPlaneSz = se->pDecodingCtx->width * se->pDecodingCtx->height;
    uvPlaneSz = se->pDecodingCtx->width * se->pDecodingCtx->height / 4;
    yPlane = (Uint8*)malloc(yPlaneSz);
    uPlane = (Uint8*)malloc(uvPlaneSz);
    vPlane = (Uint8*)malloc(uvPlaneSz);
    if (!yPlane || !uPlane || !vPlane) {
        fprintf(stderr, "Could not allocate pixel buffers - exiting\n");
        exit(1);
    }

    uvPitch = se->pDecodingCtx->width / 2;

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
        pict.linesize[0] = se->pDecodingCtx->width;
        pict.linesize[1] = uvPitch;
        pict.linesize[2] = uvPitch;

		AVPacket *pkt = av_packet_alloc();
		int ret = avcodec_send_frame(se->pEncodingCtx, frame_data->pFrame);
		if (ret < 0) {
			fprintf(stderr, "Error sending a frame for encoding\n");
			exit(1);
		}
		while (ret >= 0) {
			ret = avcodec_receive_packet(se->pEncodingCtx, pkt);
		}


        // Convert the image into YUV format that SDL uses
        sws_scale(sws_ctx, (uint8_t const * const *) frame_data->pFrame->data,
                  frame_data->pFrame->linesize, 0, se->pDecodingCtx->height, pict.data,
                  pict.linesize);
		frame_data->sdl_avframe_rescale_time_point = std::chrono::system_clock::now();

        // [SDL] update SDL overlay
        SDL_UpdateYUVTexture(
                texture,
                NULL,
                yPlane,
                se->pDecodingCtx->width,
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
	//printf("\n%s", fmt);
}

int main(int argc, char* argv[]){
    log_info("Simple GameClient has started");

    // Initialize streaming environment and threads
    log_info("Initializing streaming environment");
    StreamingEnvironment *se;
    se = (StreamingEnvironment*) av_mallocz(sizeof(StreamingEnvironment));
    se->frame_extractor_pframe_pool = simple_queue_create();
    se->frame_output_thread_queue = simple_queue_create();
	se->frame_output_thread = SDL_CreateThread(frame_output_thread, "frame_output_thread", se);
    //se->frame_extractor_thread = SDL_CreateThread(frame_extractor_thread, "frame_extractor_thread", se);
    #if defined(WIN32)
    se->gpu_frame_extractor_thread = SDL_CreateThread(gpu_frame_extractor_thread, "gpu_frame_extractor_thread", se);
    #endif
    se->pDecodingCtx = NULL;
	se->pEncodingCtx = NULL;
    se->finishing = 0;
    se->initialized = 0;
    se->screen_is_initialized = 0;	

	av_log_set_callback(my_log_callback);
	av_log_set_level(AV_LOG_VERBOSE);
	
    // [FFMPEG] Registering file formats and codecs
    log_info("Registering file formats and codecs");
    av_register_all();

	/* find the mpeg1video encoder */
	se->decoder = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!se->decoder) {
		fprintf(stderr, "Codec '%s' not found\n", "h264");
		exit(1);
	}
	se->pDecodingCtx = avcodec_alloc_context3(se->decoder);
	if (!se->pDecodingCtx) {
		fprintf(stderr, "Could not allocate video codec context\n");
		exit(1);
	}
	/* resolution must be a multiple of two */
	se->pDecodingCtx->width = SDL_WINDOW_WIDTH;
	se->pDecodingCtx->height = SDL_WINDOW_HEIGHT;

	/* emit one intra frame every ten frames
	 * check frame pict_type before passing frame
	 * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
	 * then gop_size is ignored and the output of encoder
	 * will always be I frame irrespective to gop_size
	 */
	se->pDecodingCtx->gop_size = 10;
	se->pDecodingCtx->max_b_frames = 1;
	se->pDecodingCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	//se->pCodecCtx = pCodecCtxOrig;

	////////////////////////////////////////////////////////////////

	se->encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!se->encoder) {
		fprintf(stderr, "Codec '%s' not found\n", "h264");
		exit(1);
	}
	se->pEncodingCtx = avcodec_alloc_context3(se->encoder);
	if (!se->pEncodingCtx) {
		fprintf(stderr, "Could not allocate video codec context\n");
		exit(1);
	}
	/* resolution must be a multiple of two */
	se->pEncodingCtx->width = SDL_WINDOW_WIDTH;
	se->pEncodingCtx->height = SDL_WINDOW_HEIGHT;

	/* emit one intra frame every ten frames
	 * check frame pict_type before passing frame
	 * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
	 * then gop_size is ignored and the output of encoder
	 * will always be I frame irrespective to gop_size
	 */
	se->pEncodingCtx->bit_rate = 64000;
	se->pEncodingCtx->gop_size = 10;
	se->pEncodingCtx->max_b_frames = 1;
	se->pEncodingCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	se->pEncodingCtx->time_base.den = 60;
	se->pEncodingCtx->time_base.num = 1;

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

    // Open codec
    log_info("Open codec");
	int result = avcodec_open2(se->pEncodingCtx, se->encoder, NULL);
	if (result < 0) {
		log_error("Could not open codec");
		return -1;
	}
	if (avcodec_open2(se->pDecodingCtx, se->decoder, NULL) < 0) {
		log_error("Could not open codec");
		return -1;
	}

    // Application is ready to read frame and display frames
    se->initialized = 1;

	// [FFMPEG] Initialize frame pool
	for (int i = 0; i < 60; i++) {
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

    // [FFMPEG] Free the RGB image
    while (! simple_queue_is_empty(se->frame_extractor_pframe_pool)) {
        FrameData* frame_data = (FrameData *) simple_queue_pop(se->frame_extractor_pframe_pool);
        frame_data_destroy(frame_data);
    }

    // [FFMPEG] Close the codecs
    log_info("Close the codecs");
    avcodec_close(se->pDecodingCtx);
	avcodec_close(se->pEncodingCtx);

    // [FFMPEG] Close the video file
    log_info("Close the video file");
    avformat_close_input(&se->pFormatCtx);

    log_info("Simple GameClient is exiting");

    return 0;
}
