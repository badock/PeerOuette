//
// Created by Jonathan on 4/7/19.
//

#include "inputs.h"
#include "protos/route_guide.grpc.pb.h"

using gamingstreaming::InputCommand;

bool is_cursor_initialized = false;
int last_x = -1;
int last_y = -1;

int simulate_input_event(InputCommand* cmd) {
#if defined(WIN32)
    if(cmd->event_type() == SDL_KEYDOWN || cmd->event_type() == SDL_KEYUP) {
        int key_up_down_code = cmd->event_type() == SDL_KEYUP ? KEYEVENTF_KEYUP : 0;
        int virtual_key_code = VkKeyScanA(cmd->key_code());

        // ALT
               if (cmd->scan_code() == SDL_SCANCODE_LALT) {
            virtual_key_code = VK_LMENU;
        } else if (cmd->scan_code() == SDL_SCANCODE_RALT) {
            virtual_key_code = VK_RMENU;
        }
        // SHIFT
          else if (cmd->scan_code() == SDL_SCANCODE_LSHIFT) {
            virtual_key_code = VK_LSHIFT;
        } else if (cmd->scan_code() == SDL_SCANCODE_RSHIFT) {
            virtual_key_code = VK_RSHIFT;
        }
        // CONTROL
          else if (cmd->scan_code() == SDL_SCANCODE_LCTRL) {
            virtual_key_code = VK_LCONTROL;
        } else if (cmd->scan_code() == SDL_SCANCODE_RCTRL) {
            virtual_key_code = VK_RCONTROL;
        }
        // WINDOWS
          else if (cmd->scan_code() == SDL_SCANCODE_LGUI) {
            virtual_key_code = VK_LWIN;
        } else if (cmd->scan_code() == SDL_SCANCODE_RGUI) {
           virtual_key_code = VK_RWIN;
        }
        // ARROWS
          else if (cmd->scan_code() == SDL_SCANCODE_DOWN) {
            virtual_key_code = VK_DOWN;
        } else if (cmd->scan_code() == SDL_SCANCODE_UP) {
            virtual_key_code = VK_UP;
        } else if (cmd->scan_code() == SDL_SCANCODE_LEFT) {
            virtual_key_code = VK_LEFT;
        } else if(cmd->scan_code() == SDL_SCANCODE_RIGHT) {
            virtual_key_code = VK_RIGHT;
        }

        keybd_event(virtual_key_code, 0, key_up_down_code, 0);
    } else
    if(cmd->event_type() == SDL_MOUSEBUTTONDOWN || cmd->event_type() == SDL_MOUSEBUTTONUP || cmd->event_type() == SDL_MOUSEWHEEL || cmd->event_type() == SDL_MOUSEMOTION) {
        if (! is_cursor_initialized) {
            //SetCursorPos(cmd->x(), cmd->y());
            is_cursor_initialized = true;
        } else {

            int dx = cmd->x();// - last_x;
            int dy = cmd->y();// - last_y;

            INPUT* Inputs = new INPUT();
            Inputs->type = INPUT_MOUSE;
            Inputs->mi.dx = dx;
            Inputs->mi.dy = dy;
            Inputs->mi.dwFlags = (MOUSEEVENTF_MOVE);

            SendInput(1, Inputs, sizeof(INPUT));
            delete Inputs;
        }

        last_x = cmd->x();
        last_y = cmd->y();

        //printf("> mouse moved at: (%d,%d)\n", cmd->x(), cmd->y());

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
            delete Inputs;
        }
    } else {
        log_info("unhandled event %i", cmd->event_type());
    }
#endif
    return 0;
}

mouse_info* get_mouse_info() {
    mouse_info* mouse_info_ptr = new mouse_info();

    #if defined(WIN32)
    // detect position of mouse cursor
    POINT p;
    if (GetCursorPos(&p)) {
        mouse_info_ptr->x = p.x;
        mouse_info_ptr->y = p.y;
    }
    // detect if mouse cursor is visible
    CURSORINFO cursorInfo = { 0 };
    cursorInfo.cbSize = sizeof(cursorInfo);
    if (GetCursorInfo(&cursorInfo)) {
        mouse_info_ptr->visible = (cursorInfo.flags == 1);
    }
    #endif

    return mouse_info_ptr;
}

int handle_sdl_input(StreamingEnvironment* se, SDL_Event event) {

    InputCommand* c = new InputCommand();
    c->set_command("command");
    c->set_event_type(event.type);

    bool send_event = true;

    if(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        c->set_key_code(event.key.keysym.sym);
        c->set_scan_code(event.key.keysym.scancode);
        c->set_event_type(event.type);
    }
    else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEWHEEL || event.type == SDL_MOUSEMOTION){

        int mouse_x_rel = 0;
        int mouse_y_rel = 0;

        if (event.type == SDL_MOUSEMOTION) {
            mouse_x_rel = event.motion.xrel * se->width / se->client_width;
            mouse_y_rel = event.motion.yrel * se->height / se->client_height;;
        }

        c->set_x(mouse_x_rel);
        c->set_y(mouse_y_rel);
        c->set_mouse_button(event.button.button);
    } else {
        send_event = false;
    }

    if (send_event) {
        se->input_command_queue.push(c);
    }

    return 0;
}
