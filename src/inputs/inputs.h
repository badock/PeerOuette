//
// Created by Jonathan on 4/7/19.
//


#ifndef GAMECLIENTSDL_INPUTS_H
#define GAMECLIENTSDL_INPUTS_H

#include <SDL_events.h>
#include "src/streaming/streaming.h"

int handle_sdl_input(StreamingEnvironment* se, SDL_Event event);
int simulate_input_event(InputCommand* cmd);

typedef struct mouse_info {
    int x;
    int y;
    bool visible;
    int cursor_type;
} mouse_info;

mouse_info* get_mouse_info();

#endif //GAMECLIENTSDL_INPUTS_H
