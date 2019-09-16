//
// Created by Jonathan on 2/13/19.
//
#include "src/streaming/streaming.h"

#ifndef GAMECLIENTSDL_CODEC_H
#define GAMECLIENTSDL_CODEC_H
typedef struct packet_data {
    // fields related to ffmpeg
    int size;
    uint8_t* data;
    int64_t pts;
    int64_t dts;
    int flags;
    // field related to frame management
    int frame_number;
    int packet_number;
} packet_data;

#define USE_NETWORK true

#ifdef _WIN32
// H264 (software)
#define ENCODER_NAME "libx264"
#define DECODER_NAME "h264"
#elif __APPLE__
// H264 (videotoolbox)
#define ENCODER_NAME "h264_videotoolbox"
#define DECODER_NAME "h264"

//// H264 (software)
//#define ENCODER_NAME "libx264"
//#define DECODER_NAME "h264"

//// H265 (videotoolbox)
//#define ENCODER_NAME "hevc_videotoolbox"
//#define DECODER_NAME "hevc"
#elif __linux__
//// H264 (nvenc)
//#define ENCODER_NAME "h264_nvenc"
//#define DECODER_NAME "h264"

// H264 (videotoolbox)
#define ENCODER_NAME "h264_videotoolbox"
#define DECODER_NAME "h264"

//// H264 (software)
//#define ENCODER_NAME "libx264"
//#define DECODER_NAME "h264"

//// H265 (videotoolbox)
//#define ENCODER_NAME "hevc_videotoolbox"
//#define DECODER_NAME "hevc"
#endif

// #define WIDTH 1920
// #define HEIGHT 1080
#define BITRATE 3 * 1024 * 1024
#define CRF "34"
#define GOP_SIZE 30 * 60

#endif //GAMECLIENTSDL_CODEC_H

int video_encode_thread(void *arg);
int video_decode_thread(void *arg);
