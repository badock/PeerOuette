#include "src/log/log.h"

#include <SDL.h>
#include <SDL_thread.h>

#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
/* #include <unistd.h> */
#include <sys/types.h>
#include <sys/stat.h>

// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

#undef main

char *VIDEO_FILE_PATH = "misc/bigbunny.mp4";

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

int main(int argc, char* argv[]){
    log_info("Simple GameClient has started");

    // Registering file formats and codecs
    log_info("Registering file formats and codecs");
    av_register_all();
    AVFormatContext *pFormatCtx = NULL;

    // Open video file
    log_info("Open video file: %s", VIDEO_FILE_PATH);
    if (avformat_open_input(&pFormatCtx, VIDEO_FILE_PATH, NULL, NULL) != 0 ) {
        log_error("Could not open video file: %s", VIDEO_FILE_PATH);
        return -1;
    }

    // Retrieve stream information
    log_info("Retrieve stream information");
    if(avformat_find_stream_info(pFormatCtx, NULL) < 0){
        log_error("Couldn't find stream information");
        return -1;
    }

    // Dump information about file onto standard error
    log_info("Dump information about file");
    av_dump_format(pFormatCtx, 0, argv[1], 0);

    int i, videoStream;
    AVCodecContext *pCodecCtxOrig = NULL;
    AVCodecContext *pCodecCtx = NULL;

    // Find the first video stream
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


    // Get a pointer to the codec context for the video stream
    pCodecCtx=pFormatCtx->streams[videoStream]->codec;

    AVCodec *pCodec = NULL;

    // Find the decoder for the video stream
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

    // Allocate video frame
    log_info("Allocate video frame");
    AVFrame *pFrame = NULL;
    pFrame=av_frame_alloc();

    // Allocate an AVFrame structure
    log_info("Allocate an AVFrame structure");
    AVFrame *pFrameRGB = NULL;
    pFrameRGB=av_frame_alloc();
    if(pFrameRGB==NULL)
        return -1;

    // Determine required buffer size and allocate buffer
    log_info("Determine required buffer size and allocate buffer");
    uint8_t *buffer = NULL;
    int numBytes;
    numBytes=avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width,
                                pCodecCtx->height);
    buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

    // Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
    // of AVPicture
    log_info("Assign appropriate parts of buffer to image planes in pFrameRGB");
    avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_RGB24,
                   pCodecCtx->width, pCodecCtx->height);

    // Initialize SWS context for software scaling
    log_info("Initialize SWS context for software scaling");
    struct SwsContext *sws_ctx = NULL;
    int frameFinished;
    AVPacket packet;
    sws_ctx = sws_getContext(pCodecCtx->width,
                             pCodecCtx->height,
                             pCodecCtx->pix_fmt,
                             pCodecCtx->width,
                             pCodecCtx->height,
                             AV_PIX_FMT_RGB24,
                             SWS_BILINEAR,
                             NULL,
                             NULL,
                             NULL
    );


    // Reading frames
    log_info("Reading frames");
    i=0;
    while(av_read_frame(pFormatCtx, &packet)>=0) {
        // Is this a packet from the video stream?
        if(packet.stream_index==videoStream) {
            // Decode video frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

            // Did we get a video frame?
            if(frameFinished) {
                // Convert the image from its native format to RGB
                sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
                          pFrame->linesize, 0, pCodecCtx->height,
                          pFrameRGB->data, pFrameRGB->linesize);

                // Save the frame to disk
                ++i;
                SaveFrame(pFrameRGB, pCodecCtx->width,
                          pCodecCtx->height, i);
            }
        }

        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }

    // Free the RGB image
    log_info("Free the RGB image");
    av_free(buffer);
    av_free(pFrameRGB);

    // Free the YUV frame
    log_info("Free the YUV frame");
    av_free(pFrame);

    // Close the codecs
    log_info("Close the codecs");
    avcodec_close(pCodecCtx);
    avcodec_close(pCodecCtxOrig);

    // Close the video file
    log_info("Close the video file");
    avformat_close_input(&pFormatCtx);

    log_info("Simple GameClient is exiting");

    return 0;
}
