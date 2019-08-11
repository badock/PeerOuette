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
#endif //GAMECLIENTSDL_CODEC_H

int video_encode_thread(void *arg);
int video_decode_thread(void *arg);
