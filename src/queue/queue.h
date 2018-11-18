//
// Created by Jonathan on 11/18/18.
//

#include <SDL_thread.h>

#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>

#ifndef GAMECLIENTSDL_QUEUE_H
#define GAMECLIENTSDL_QUEUE_H
typedef struct QueueNode {
    void* ptr;
    struct QueueNode* next_queue_node;
} QueueNode;

typedef struct SimpleQueue {
    struct QueueNode* next;
    SDL_mutex* mutex;
    SDL_cond* cond;
} SimpleQueue;

SimpleQueue* simple_queue_create();
int simple_queue_destroy(SimpleQueue *simple_queue);
void* simple_queue_pop(SimpleQueue *simple_queue);
void simple_queue_push(SimpleQueue *simple_queue, void *element);
int simple_queue_is_empty(SimpleQueue *simple_queue);
int simple_queue_length(SimpleQueue *simple_queue);
#endif //GAMECLIENTSDL_QUEUE_H
