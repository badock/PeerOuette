//
// Created by Jonathan on 3/26/19.
//

#ifndef GAMECLIENTSDL_EXTRACTORS_H
#define GAMECLIENTSDL_EXTRACTORS_H

#include "src/streaming/streaming.h"
#if defined(WIN32)
#include "src/dxcapture/capture.h"
#include "src/texture_converter/TextureConverter.h"
#else
#include <unistd.h>
#endif

#if defined(WIN32)
int gpu_frame_extractor_thread(void *arg);
#endif

int frame_extractor_thread(void *arg);

#endif //GAMECLIENTSDL_EXTRACTORS_H
