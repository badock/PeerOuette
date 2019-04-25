//
// Created by Jonathan on 3/26/19.
//

#include "output.h"


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
            se->width,
            se->height
    );
    if (!texture) {
        fprintf(stderr, "SDL: could not create texture - exiting\n");
        exit(1);
    }

    // Create the mouse cursor display on the SDL client
    SDL_Surface* image = SDL_LoadBMP("assets/cursor.bmp");
    if(!image)
    {
        printf("Error while loading the image: %s",SDL_GetError());
        return -1;
    }
    image->h = 20;
    image->w = 20;
    SDL_Texture* mouse_cursor_icon_texture = SDL_CreateTextureFromSurface(se->renderer, image);
    // let's say the texture is 16*16px
    // If SDL_SetHint was called properly, it will scale without blurring.
//    SDL_RenderCopy(se->renderer, mouse_cursor_icon_texture, nullptr, &dest);
    SDL_FreeSurface(image);

    // initialize SWS context for software scaling
    sws_ctx = sws_getContext(se->width,
                             se->height,
                             se->format,
                             se->width,
                             se->height,
                             AV_PIX_FMT_YUV420P,
                             SWS_BILINEAR,
                             NULL,
                             NULL,
                             NULL);


    // [SDL] set up YV12 pixel array (12 bits per pixel)
    Uint8 *yPlane, *uPlane, *vPlane;
    size_t yPlaneSz, uvPlaneSz;
    int uvPitch;
    yPlaneSz = se->width * se->height;
    uvPlaneSz = se->width * se->height / 4;
    yPlane = (Uint8*)malloc(yPlaneSz);
    uPlane = (Uint8*)malloc(uvPlaneSz);
    vPlane = (Uint8*)malloc(uvPlaneSz);
    if (!yPlane || !uPlane || !vPlane) {
        fprintf(stderr, "Could not allocate pixel buffers - exiting\n");
        exit(1);
    }

    uvPitch = se->width / 2;

    // Grab mouse
    SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_SetWindowGrab(se->screen, SDL_TRUE);

    // Screen is ready
    log_info("[SDL] screen is ready");
    se->screen_is_initialized = 1;

    int i = 0;
    while(se->finishing != 1) {
        //log_info("frame_output_thread: %i elements in queue", simple_queue_length(se->frame_output_thread_queue));
        auto frame_data = (FrameData*) se->frame_output_thread_queue.pop();
        frame_data->sdl_received_time_point = std::chrono::system_clock::now();

        // [SDL] Create an AV Picture
        AVPicture pict;
        pict.data[0] = yPlane;
        pict.data[1] = uPlane;
        pict.data[2] = vPlane;
        pict.linesize[0] = se->width;
        pict.linesize[1] = uvPitch;
        pict.linesize[2] = uvPitch;

        // Convert the image into YUV format that SDL uses
        sws_scale(sws_ctx,
                  (uint8_t const * const *) frame_data->pFrame->data,
                  frame_data->pFrame->linesize,
                  0,
                  se->height,
                  pict.data,
                  pict.linesize);
        frame_data->sdl_avframe_rescale_time_point = std::chrono::system_clock::now();

        // [SDL] update SDL overlay
        SDL_UpdateYUVTexture(
                texture,
                nullptr,
                yPlane,
                se->width,
                uPlane,
                uvPitch,
                vPlane,
                uvPitch
        );
        SDL_RenderClear(se->renderer);
        SDL_RenderCopy(se->renderer, texture, nullptr, nullptr);

        float ratio_height = 1.0 * se->client_height / se->height;
        float ratio_width = 1.0 * se->client_width / se->width;

        SDL_Rect dest = {
                .x = int(se->client_mouse_x * ratio_width),
                .y = int(se->client_mouse_y * ratio_height),
                .w = 20,
                .h = 20
        };
        SDL_RenderCopy(se->renderer, mouse_cursor_icon_texture, nullptr, &dest);
        SDL_RenderPresent(se->renderer);
        frame_data->sdl_displayed_time_point = std::chrono::system_clock::now();

        i++;

        frame_data_debug(frame_data);
        se->frame_extractor_pframe_pool.push(frame_data);

        // Put processed frames back in the extractor pool
        while (se->frame_output_thread_queue.size() > 0) {
            auto* processed_frame_data = se->frame_output_thread_queue.pop();
            se->frame_extractor_pframe_pool.push(processed_frame_data);
        }
    }

    // [SDL] Cleaning SDL data
    log_info("Cleaning SDL data");
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(se->renderer);
    SDL_DestroyWindow(se->screen);

    // [SDL] Free the YUV image
    log_info("Free the YUV image");
    free(yPlane);
    free(uPlane);
    free(vPlane);

    return 0;
}
