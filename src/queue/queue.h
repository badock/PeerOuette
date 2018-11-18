//
// Created by Jonathan on 11/18/18.
//

#include <SDL_thread.h>

#ifndef GAMECLIENTSDL_QUEUE_H
#define GAMECLIENTSDL_QUEUE_H
typedef struct QueueNode {
    void* ptr;
    int type;
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
#endif //GAMECLIENTSDL_QUEUE_H
