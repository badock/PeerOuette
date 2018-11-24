//
// Created by Jonathan on 11/18/18.
//

#include "queue.h"
#include <stdlib.h>

SimpleQueue* simple_queue_create() {
    SimpleQueue* new_simple_queue = (SimpleQueue*) malloc(sizeof(SimpleQueue));
    new_simple_queue->mutex = SDL_CreateMutex();
    new_simple_queue->cond = SDL_CreateCond();
    new_simple_queue->next = NULL;
    return new_simple_queue;
}

int simple_queue_destroy(SimpleQueue *simple_queue) {
    free(simple_queue);
    return 1;
}

void* simple_queue_pop(SimpleQueue *simple_queue) {
    void* result;
    QueueNode* current_queue_node;
    // Acquire the queue's lock
    SDL_LockMutex(simple_queue->mutex);
    // Queue is empty
    while (simple_queue->next == NULL) {
        // Queue is empty -> wait for a signal that someone has pushed in the queue
        SDL_CondWait(simple_queue->cond, simple_queue->mutex);
    }
    // Get the current first element in the queue
    current_queue_node = simple_queue->next;
    result = current_queue_node->ptr;
    // Second element of the queue (or NULL) is now first
    simple_queue->next = current_queue_node->next_queue_node;
    // Release the memory of the popped element
    free(current_queue_node);
    // Release the lock on the queue
    SDL_UnlockMutex(simple_queue->mutex);
    return result;
}

void simple_queue_push(SimpleQueue *simple_queue, void *element) {
    // Acquire the queue's lock
    SDL_LockMutex(simple_queue->mutex);
    // Allocate memory to store a new element of the queue
    QueueNode* new_queue_node = (QueueNode*) malloc(sizeof(QueueNode));
    new_queue_node->ptr = element;
    new_queue_node->next_queue_node = NULL;
    // Add the new element at the end of the queue
    if (simple_queue->next == NULL) {
        simple_queue->next = new_queue_node;
    } else {
        // Create an "iterator" on queue nodes, which is initialized to the first node
        QueueNode* iter_queue_node = simple_queue->next;
        // We search for the last element of the queue
        while(iter_queue_node->next_queue_node != NULL) {
            iter_queue_node = iter_queue_node->next_queue_node;
        }
        // The new node become the last node of the queue
        iter_queue_node->next_queue_node = new_queue_node;
    }
    // Send a signal that something has been pushed to the queue
    SDL_CondSignal(simple_queue->cond);
    // Release the lock on the queue
    SDL_UnlockMutex(simple_queue->mutex);
}

int simple_queue_is_empty(SimpleQueue *simple_queue) {
    return simple_queue->next == NULL;
}

int simple_queue_length(SimpleQueue *simple_queue) {
    if (simple_queue->next == NULL) {
        return 0;
    } else {
        // Create an "iterator" on queue nodes, which is initialized to the first node
        QueueNode* iter_queue_node = simple_queue->next;
        int length = 1;
        // We search for the last element of the queue
        while(iter_queue_node->next_queue_node != NULL) {
            iter_queue_node = iter_queue_node->next_queue_node;
            length += 1;
        }
        return length;
    }
}