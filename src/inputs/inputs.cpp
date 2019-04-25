//
// Created by Jonathan on 4/7/19.
//

#include "inputs.h"
#include "src/network/grpc/route_guide.grpc.pb.h"

using gamingstreaming::InputCommand;

bool is_cursor_initialized = false;
int last_x = -1;
int last_y = -1;

int simulate_input_event(InputCommand* cmd) {
    if(cmd->event_type() == SDL_KEYDOWN || cmd->event_type() == SDL_KEYUP) {
        log_info("keyboard event");
    } else
    if(cmd->event_type() == SDL_MOUSEBUTTONDOWN || cmd->event_type() == SDL_MOUSEBUTTONUP || cmd->event_type() == SDL_MOUSEWHEEL || cmd->event_type() == SDL_MOUSEMOTION) {
        log_info("mouse event");
        if (! is_cursor_initialized) {
            SetCursorPos(cmd->x(), cmd->y());
            is_cursor_initialized = true;
        } else {

            int dx = cmd->x() - last_x;
            int dy = cmd->y() - last_y;

            INPUT* Inputs = new INPUT();
            Inputs->type = INPUT_MOUSE;
            Inputs->mi.dx = dx;
            Inputs->mi.dy = dy;
            Inputs->mi.dwFlags = (MOUSEEVENTF_MOVE);

            SendInput(1, Inputs, sizeof(INPUT));
        }

        last_x = cmd->x();
        last_y = cmd->y();

        printf("> mouse moved at: (%d,%d)\n", cmd->x(), cmd->y());

        if (cmd->event_type() != SDL_MOUSEMOTION) {

           INPUT* Inputs = new INPUT();
           Inputs->type = INPUT_MOUSE;

            if (cmd->event_type() == SDL_MOUSEBUTTONDOWN) {
                printf("> button mouse (%d) down at: (%d,%d)\n", cmd->mouse_button(), cmd->x(), cmd->y());
                Inputs->mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            }
            else if (cmd->event_type() == SDL_MOUSEBUTTONUP) {
                printf("> button mouse (%d) up at: (%d,%d)\n", cmd->mouse_button(), cmd->x(), cmd->y());
                Inputs->mi.dwFlags = MOUSEEVENTF_LEFTUP;
            }

           SendInput(3, Inputs, sizeof(INPUT));
        }
    } else {
        log_info("unhandled event %i", cmd->event_type());
    }

    return 0;
}

int handle_sdl_input(StreamingEnvironment* se, SDL_Event event) {


    InputCommand* c = new InputCommand();
    c->set_command("command");
    c->set_event_type(event.type);

    bool send_event = false;
//    return c;

    if(event.type == SDL_KEYDOWN ) {
        c->set_key_code(event.key.keysym.sym);

//        switch( event.key.keysym.sym ) {
//            case SDLK_UP:
//                puts("up arrow");
//                break;
//            case SDLK_DOWN://for full list of key names, http://www.libsdl.org/release/SDL-1.2.15/docs/html/sdlkey.html
//                puts("down arrow");
//
//                SDL_KeyboardEvent event2;
//                SDL_KeyboardEvent* e = (SDL_KeyboardEvent*) &event;
//
//                event2.type = SDL_KEYDOWN;
//                event2.timestamp = e->timestamp + 1;
//                event2.windowID = e->windowID;
//                event2.state = SDL_PRESSED;
//
//                event2.keysym.scancode = e->keysym.scancode; // from SDL_Keysym
//                event2.keysym.sym = SDLK_UP;
//                event2.keysym.mod = 0; // from SDL_Keymod
//
//                SDL_PushEvent((SDL_Event*) &event2);
//
//                break;
//        }

        send_event = true;
    }
    else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEWHEEL || event.type == SDL_MOUSEMOTION){
        int local_window_x, local_window_y;

        SDL_GetMouseState(&local_window_x, &local_window_y);

        int remote_window_x = local_window_x * se->width / se->client_width;
        int remote_window_y = local_window_y * se->height / se->client_height;

        c->set_x(remote_window_x);
        c->set_y(remote_window_y);
        c->set_mouse_button(event.button.button);

//        if (event.type == SDL_MOUSEBUTTONDOWN) {
//            auto button_mouse = (event.button.button == SDL_BUTTON_LEFT)? "left" : "right";
//            printf("> button mouse (%s) down at: (%d,%d) local:(%d,%d)\n", button_mouse, remote_window_x, remote_window_y, local_window_x, local_window_y);
//        }
//        else if (event.type == SDL_MOUSEBUTTONUP) {
//            auto button_mouse = (event.button.button == SDL_BUTTON_LEFT)? "left" : "right";
//            printf("> button mouse (%s) up at: (%d,%d) local:(%d,%d)\n", button_mouse, remote_window_x, remote_window_y, local_window_x, local_window_y);
//        }
//        else if(event.type == SDL_MOUSEMOTION) {
//            printf("> mouse moved at: (%d,%d) local:(%d,%d)\n", remote_window_x, remote_window_y, local_window_x, local_window_y);
//        }

        send_event = true;
    } else {

    }

    if (send_event) {
        se->input_command_queue.push(c);
    }

    return 0;
}
