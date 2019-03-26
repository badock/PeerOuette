//
// Created by Jonathan on 3/26/19.
//

#ifndef GAMECLIENTSDL_OUTPUT_H
#define GAMECLIENTSDL_OUTPUT_H

#include "src/streaming/streaming.h"

#include "src/network/network.h"
#include "src/codec/codec.h"
#if defined(WIN32)
#include "src/dxcapture/capture.h"
#include "src/texture_converter/TextureConverter.h"
#else
#include <unistd.h>
#endif

//#undef main


int frame_output_thread(void *arg);

#endif //GAMECLIENTSDL_OUTPUT_H
