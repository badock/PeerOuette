#include "src/log/log.h"

#include <SDL.h>
#include <SDL_thread.h>

#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <stdlib.h>
#include "src/queue/queue.h"

// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

#if defined(WIN32)
#include <Windows.h>
void usleep(unsigned int usec)
{
	Sleep(500);
}
#else
#include <unistd.h>
#endif

#undef main

char *VIDEO_FILE_PATH = "misc/bigbunny.mp4";

typedef struct StreamingEnvironment {
    SDL_Thread *frame_extractor_tid;
    SDL_Thread *frame_encoder_tid;
    SDL_Thread *frame_decoder_tid;
    SDL_Thread *frame_output_tid;
    SimpleQueue *simpleQueue;
} StreamingEnvironment;

StreamingEnvironment *global_streaming_environment;

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
    FILE *pFile;
    char szFilename[32];
    int  y;

    // Ensure tmp folder exists
    struct stat st = {0};
    if (stat("tmp", &st) == -1) {
        mkdir("tmp", 0700);
    }

    // Open file
    sprintf(szFilename, "tmp/frame%d.ppm", iFrame);
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

int frame_extractor_thread(void *arg) {
    StreamingEnvironment *se = (StreamingEnvironment*) arg;
    for(;;) {
        log_info("frame_extractor_thread: plop");
        for (int i=0; i<3; i++) {
            int* ptr_i = malloc(sizeof(int));
            *ptr_i = i;
            simple_queue_push(se->simpleQueue, ptr_i);
        }
        sleep(1);
    }
    return 0;
}

int frame_output_thread(void *arg) {
    StreamingEnvironment *se = (StreamingEnvironment*) arg;
    int i = 0;
    for(;;) {
        log_info("frame_output_thread: plop");
        int* ptr_i = simple_queue_pop(se->simpleQueue);
        log_info("i: %i -> %i", i, *ptr_i);
        i++;
//        sleep(1);
    }
    return 0;
}

int main(int argc, char* argv[]){
    log_info("Simple GameClient has started");

    // Initialize streaming environment and threads
    log_info("Initializing streaming environment");
    StreamingEnvironment *se;
    se = av_mallocz(sizeof(StreamingEnvironment));
    se->simpleQueue = simple_queue_create();
    se->frame_extractor_tid = SDL_CreateThread(frame_extractor_thread, "frame_extractor_thread", se);
    se->frame_output_tid = SDL_CreateThread(frame_output_thread, "frame_output_thread", se);

    // [SDL] Initializing SDL library
    log_info("Initializing SDL library");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        exit(1);
    }

    // [FFMPEG] Registering file formats and codecs
    log_info("Registering file formats and codecs");
    av_register_all();
    AVFormatContext *pFormatCtx = NULL;

    // [FFMPEG] Open video file
    log_info("Open video file: %s", VIDEO_FILE_PATH);
    if (avformat_open_input(&pFormatCtx, VIDEO_FILE_PATH, NULL, NULL) != 0 ) {
        log_error("Could not open video file: %s", VIDEO_FILE_PATH);
        return -1;
    }

    // [FFMPEG] Retrieve stream information
    log_info("Retrieve stream information");
    if(avformat_find_stream_info(pFormatCtx, NULL) < 0){
        log_error("Couldn't find stream information");
        return -1;
    }

    // [FFMPEG] Dump information about file onto standard error
    log_info("Dump information about file");
    av_dump_format(pFormatCtx, 0, VIDEO_FILE_PATH, 0);

    int i, videoStream;
    AVCodecContext *pCodecCtxOrig = NULL;
    AVCodecContext *pCodecCtx = NULL;

    // [FFMPEG] Find the first video stream
    log_info("Find the first video stream");
    videoStream=-1;
    for(i=0; i<pFormatCtx->nb_streams; i++)
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
            videoStream=i;
            break;
        }
    if(videoStream==-1){
        log_error("Didn't find a video stream");
        return -1;
    }

    // [FFMPEG] Get a pointer to the codec context for the video stream
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;

    AVCodec *pCodec = NULL;

    // [FFMPEG] Find the decoder for the video stream
    log_info("Find the decoder for the video stream");

    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec==NULL) {
        log_error("Unsupported codec!\n");
        return -1; // Codec not found
    }

//    // Copy context
//    log_info("Copy context");
//    pCodecCtx = avcodec_alloc_context3(pCodec);
//    if (avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {
//        log_error("Couldn't copy codec context");
//        return -1; // Error copying codec context
//    }

    // Open codec
    log_info("Open codec");
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        log_error("Could not open codec");
        return -1;
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
            pCodecCtx->width,
            pCodecCtx->height,
            screen_flags
    );
    SDL_ShowWindow(screen);

    if (!screen) {
        fprintf(stderr, "SDL: could not create window - exiting\n");
        exit(1);
    }

    SDL_Renderer *renderer;
    renderer = SDL_CreateRenderer(screen, -1, 0);
    if (!renderer) {
        fprintf(stderr, "SDL: could not create renderer - exiting\n");
        exit(1);
    }


    // [SDL] Create a YUV overlay
    log_info("Creating a YUV overlay");
    SDL_Texture *texture;
    struct SwsContext *sws_ctx = NULL;

    texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_YV12,
            SDL_TEXTUREACCESS_STREAMING,
            pCodecCtx->width,
            pCodecCtx->height
    );
    if (!texture) {
        fprintf(stderr, "SDL: could not create texture - exiting\n");
        exit(1);
    }

    // initialize SWS context for software scaling
    sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                             pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
                             AV_PIX_FMT_YUV420P,
                             SWS_BILINEAR,
                             NULL,
                             NULL,
                             NULL);

    // [FFMPEG] Allocate video frame
    log_info("Allocate video frame");
    AVFrame *pFrame = NULL;
    pFrame=av_frame_alloc();

    // [FFMPEG] Allocate an AVFrame structure
    log_info("Allocate an AVFrame structure");
    AVFrame *pFrameRGB = NULL;
    pFrameRGB=av_frame_alloc();
    if(pFrameRGB==NULL)
        return -1;

    // [FFMPEG] Determine required buffer size and allocate buffer
    log_info("Determine required buffer size and allocate buffer");
    uint8_t *buffer = NULL;
    int numBytes;
    numBytes=avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width,
                                pCodecCtx->height);
    buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

    // [FFMPEG] Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
    // of AVPicture
    log_info("Assign appropriate parts of buffer to image planes in pFrameRGB");
    avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_RGB24,
                   pCodecCtx->width, pCodecCtx->height);

//    // [FFMPEG] Initialize SWS context for software scaling
//    log_info("Initialize SWS context for software scaling");
////    struct SwsContext *sws_ctx = NULL;
//    sws_ctx = sws_getContext(pCodecCtx->width,
//                             pCodecCtx->height,
//                             pCodecCtx->pix_fmt,
//                             pCodecCtx->width,
//                             pCodecCtx->height,
//                             AV_PIX_FMT_RGB24,
//                             SWS_BILINEAR,
//                             NULL,
//                             NULL,
//                             NULL
//    );


    // [SDL] set up YV12 pixel array (12 bits per pixel)
    Uint8 *yPlane, *uPlane, *vPlane;
    size_t yPlaneSz, uvPlaneSz;
    int uvPitch;
    yPlaneSz = pCodecCtx->width * pCodecCtx->height;
    uvPlaneSz = pCodecCtx->width * pCodecCtx->height / 4;
    yPlane = (Uint8*)malloc(yPlaneSz);
    uPlane = (Uint8*)malloc(uvPlaneSz);
    vPlane = (Uint8*)malloc(uvPlaneSz);
    if (!yPlane || !uPlane || !vPlane) {
        fprintf(stderr, "Could not allocate pixel buffers - exiting\n");
        exit(1);
    }

    // [FFMPEG] Reading frames
    log_info("Reading frames");
    int frameFinished;
    SDL_Event event;
    AVPacket packet;
    i=0;
    uvPitch = pCodecCtx->width / 2;
    while(av_read_frame(pFormatCtx, &packet)>=0) {
//        SDL_PumpEvents();
        // Is this a packet from the video stream?
        if(packet.stream_index==videoStream) {
            // Decode video frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

            // Did we get a video frame?
            if(frameFinished) {
                // [SDL] Create an AV Picture
                AVPicture pict;
                pict.data[0] = yPlane;
                pict.data[1] = uPlane;
                pict.data[2] = vPlane;
                pict.linesize[0] = pCodecCtx->width;
                pict.linesize[1] = uvPitch;
                pict.linesize[2] = uvPitch;

                // Convert the image into YUV format that SDL uses
                sws_scale(sws_ctx, (uint8_t const * const *) pFrame->data,
                          pFrame->linesize, 0, pCodecCtx->height, pict.data,
                          pict.linesize);

                // Save the frame to disk
                ++i;
//                SaveFrame(pFrameRGB, pCodecCtx->width,
//                          pCodecCtx->height, i);


                // [SDL] update SDL overlay
                SDL_UpdateYUVTexture(
                        texture,
                        NULL,
                        yPlane,
                        pCodecCtx->width,
                        uPlane,
                        uvPitch,
                        vPlane,
                        uvPitch
                );
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, NULL, NULL);
                SDL_RenderPresent(renderer);

                usleep(33.333 * 1000);
            }

        }

        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);

        // [SDL] handle events
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    SDL_DestroyTexture(texture);
                    SDL_DestroyRenderer(renderer);
                    SDL_DestroyWindow(screen);
                    SDL_Quit();
                    exit(0);
                    break;
                default:
                    break;
            }
        }
    }

    // [SDL] Free the YUV image
    log_info("Free the YUV image");
    av_frame_free(&pFrame);
    free(yPlane);
    free(uPlane);
    free(vPlane);

    // [FFMPEG] Free the RGB image
    log_info("Free the RGB image");
    av_free(buffer);
    av_free(pFrameRGB);

    // [FFMPEG] Free the YUV frame
    log_info("Free the YUV frame");
    av_free(pFrame);

    // [FFMPEG] Close the codecs
    log_info("Close the codecs");
    avcodec_close(pCodecCtx);
    avcodec_close(pCodecCtxOrig);

    // [FFMPEG] Close the video file
    log_info("Close the video file");
    avformat_close_input(&pFormatCtx);

    log_info("Simple GameClient is exiting");

    return 0;
}
