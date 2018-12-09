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

char *VIDEO_FILE_PATH = "misc/bigbunny.mp4";

StreamingEnvironment *global_streaming_environment;

void frame_data_reset_time_points(FrameData* frame_data) {
	frame_data->life_started_time_point = std::chrono::high_resolution_clock::now();
	frame_data->dxframe_acquired_time_point = std::chrono::high_resolution_clock::now();
	frame_data->dxframe_processed_time_point = std::chrono::high_resolution_clock::now();
	frame_data->avframe_produced_time_point = std::chrono::high_resolution_clock::now();
	frame_data->sdl_received_time_point = std::chrono::high_resolution_clock::now();
	frame_data->sdl_avframe_rescale_time_point = std::chrono::high_resolution_clock::now();
	frame_data->sdl_displayed_time_point = std::chrono::high_resolution_clock::now();
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

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame, int debug) {
    FILE *pFile;
    char szFilename[32];
    int  y;

    // Ensure tmp folder exists
    struct stat st = {0};
    if (stat("tmp", &st) == -1) {
        #if defined(WIN32) && __MINGW32__
        //mkdir("tmp");
        # else
        //mkdir("tmp", 0700);
        #endif
    }

    // Open file
    if (debug) {
        sprintf(szFilename, "tmp/frame_%d_debug.ppm", iFrame);
    } else {
        sprintf(szFilename, "tmp/frame_%d.ppm", iFrame);
    }
    pFile=fopen(szFilename, "wb");
    if(pFile==NULL)
        return;

    // Write header
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);

    // Write pixel data
    for(y=0; y<height; y++)
        fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);

    // Close file
    fclose(pFile);
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
		ffmpeg_frame_data->dxframe_processed_time_point = std::chrono::high_resolution_clock::now();

		int result;
		////result = get_pixels(&cc, ffmpeg_frame_data);
		result = get_pixels_yuv420p(&cc, ffmpeg_frame_data);
		ffmpeg_frame_data->avframe_produced_time_point = std::chrono::high_resolution_clock::now();

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
            se->pCodecCtx->width,
            se->pCodecCtx->height
    );
    if (!texture) {
        fprintf(stderr, "SDL: could not create texture - exiting\n");
        exit(1);
    }

    // initialize SWS context for software scaling
    sws_ctx = sws_getContext(se->pCodecCtx->width,
                             se->pCodecCtx->height,
                             se->pCodecCtx->pix_fmt,
                             se->pCodecCtx->width,
                             se->pCodecCtx->height,
                             AV_PIX_FMT_YUV420P,
                             SWS_BILINEAR,
                             NULL,
                             NULL,
                             NULL);


    // [SDL] set up YV12 pixel array (12 bits per pixel)
    Uint8 *yPlane, *uPlane, *vPlane;
    size_t yPlaneSz, uvPlaneSz;
    int uvPitch;
    yPlaneSz = se->pCodecCtx->width * se->pCodecCtx->height;
    uvPlaneSz = se->pCodecCtx->width * se->pCodecCtx->height / 4;
    yPlane = (Uint8*)malloc(yPlaneSz);
    uPlane = (Uint8*)malloc(uvPlaneSz);
    vPlane = (Uint8*)malloc(uvPlaneSz);
    if (!yPlane || !uPlane || !vPlane) {
        fprintf(stderr, "Could not allocate pixel buffers - exiting\n");
        exit(1);
    }

    uvPitch = se->pCodecCtx->width / 2;

    // Screen is ready
    log_info("[SDL] screen is ready");
    se->screen_is_initialized = 1;

    int i = 0;
    while(se->finishing != 1) {
        //log_info("frame_output_thread: %i elements in queue", simple_queue_length(se->frame_output_thread_queue));
        FrameData* frame_data = (FrameData*) simple_queue_pop(se->frame_output_thread_queue);
		frame_data->sdl_received_time_point = std::chrono::high_resolution_clock::now();

        // [SDL] Create an AV Picture
        AVPicture pict;
        pict.data[0] = yPlane;
        pict.data[1] = uPlane;
        pict.data[2] = vPlane;
        pict.linesize[0] = se->pCodecCtx->width;
        pict.linesize[1] = uvPitch;
        pict.linesize[2] = uvPitch;

        // Convert the image into YUV format that SDL uses
        sws_scale(sws_ctx, (uint8_t const * const *) frame_data->pFrame->data,
                  frame_data->pFrame->linesize, 0, se->pCodecCtx->height, pict.data,
                  pict.linesize);
		frame_data->sdl_avframe_rescale_time_point = std::chrono::high_resolution_clock::now();

        // [SDL] update SDL overlay
        SDL_UpdateYUVTexture(
                texture,
                NULL,
                yPlane,
                se->pCodecCtx->width,
                uPlane,
                uvPitch,
                vPlane,
                uvPitch
        );
        SDL_RenderClear(se->renderer);
        SDL_RenderCopy(se->renderer, texture, NULL, NULL);
        SDL_RenderPresent(se->renderer);
		frame_data->sdl_displayed_time_point = std::chrono::high_resolution_clock::now();
		
		//frame_data_destroy(frame_data);

        i++;

		frame_data_debug(frame_data);
		simple_queue_push(se->frame_extractor_pframe_pool, frame_data);

		while (simple_queue_length(se->frame_output_thread_queue) > 0) {
			//log_info("frame_output_thread: %i elements in queue", simple_queue_length(se->frame_output_thread_queue));
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

int frame_extractor_thread(void *arg) {
    StreamingEnvironment *se = (StreamingEnvironment*) arg;

	while (!se->initialized) {
		usleep(30 * 1000);
	}

    // [FFMPEG] Reading frames
    log_info("Reading frames");
    SDL_Event event;
    AVPacket packet;
    int i=0;

    while(! se->screen_is_initialized) {
        usleep(30 * 1000);
    }

    int frameFinished;
    while(av_read_frame(se->pFormatCtx, &packet)>=0) {
        // Is this a packet from the video stream?
        if(packet.stream_index == se->videoStream) {
            // [FFMPEG] Allocate video frame
            log_info("frame_extractor_pframe_pool: %i elements in queue", simple_queue_length(se->frame_extractor_pframe_pool));
            FrameData* frame_data = (FrameData *) simple_queue_pop(se->frame_extractor_pframe_pool);
            log_info("WRITE ====> %i (%i)", frame_data->id, i);

            // Decode video frame
            avcodec_decode_video2(se->pCodecCtx, frame_data->pFrame, &frameFinished, &packet);

            // Did we get a video frame?
            if(frameFinished) {
                // Push frame to the output_video thread
                AVFrame* old_pframe = frame_data->pFrame;
                frame_data->pFrame = av_frame_clone(frame_data->pFrame);
                simple_queue_push(se->frame_output_thread_queue, frame_data);
                av_free(old_pframe);
                i++;
				
				usleep(33.333 * 1000);
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
	se->frame_output_thread = SDL_CreateThread(frame_output_thread, "frame_output_thread", se);
    //se->frame_extractor_thread = SDL_CreateThread(frame_extractor_thread, "frame_extractor_thread", se);
    #if defined(WIN32)
    se->gpu_frame_extractor_thread = SDL_CreateThread(gpu_frame_extractor_thread, "gpu_frame_extractor_thread", se);
    #endif
    se->pCodecCtx = NULL;
    se->finishing = 0;
    se->initialized = 0;
    se->screen_is_initialized = 0;

    // [FFMPEG] Registering file formats and codecs
    log_info("Registering file formats and codecs");
    av_register_all();
//    AVFormatContext *pFormatCtx = NULL;

    // [FFMPEG] Open video file
    log_info("Open video file: %s", VIDEO_FILE_PATH);
    if (avformat_open_input(&se->pFormatCtx, VIDEO_FILE_PATH, NULL, NULL) != 0 ) {
        log_error("Could not open video file: %s", VIDEO_FILE_PATH);
        return -1;
    }

    // [FFMPEG] Retrieve stream information
    log_info("Retrieve stream information");
    if(avformat_find_stream_info(se->pFormatCtx, NULL) < 0){
        log_error("Couldn't find stream information");
        return -1;
    }

    // [FFMPEG] Dump information about file onto standard error
    log_info("Dump information about file");
    av_dump_format(se->pFormatCtx, 0, VIDEO_FILE_PATH, 0);

    int i;
    AVCodecContext *pCodecCtxOrig = NULL;
    se->pCodecCtx = NULL;

    // [FFMPEG] Find the first video stream
    log_info("Find the first video stream");
    se->videoStream=-1;
    for(i=0; i<se->pFormatCtx->nb_streams; i++)
        if(se->pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
            se->videoStream=i;
            break;
        }
    if(se->videoStream==-1){
        log_error("Didn't find a video stream");
        return -1;
    }

    // [FFMPEG] Get a pointer to the codec context for the video stream
    se->pCodecCtx = se->pFormatCtx->streams[se->videoStream]->codec;

    AVCodec *pCodec = NULL;

    // [FFMPEG] Find the decoder for the video stream
    log_info("Find the decoder for the video stream");

    pCodec=avcodec_find_decoder(se->pCodecCtx->codec_id);
    if (pCodec==NULL) {
        log_error("Unsupported codec!\n");
        return -1; // Codec not found
    }

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
            se->pCodecCtx->width,
            se->pCodecCtx->height,
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
    if (avcodec_open2(se->pCodecCtx, pCodec, NULL) < 0) {
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
    avcodec_close(se->pCodecCtx);
    avcodec_close(pCodecCtxOrig);

    // [FFMPEG] Close the video file
    log_info("Close the video file");
    avformat_close_input(&se->pFormatCtx);

    log_info("Simple GameClient is exiting");

    return 0;
}
