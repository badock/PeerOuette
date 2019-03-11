//
// Created by Jonathan on 11/18/18.
//

#include <SDL_thread.h>

#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <vector>

#ifndef GAMECLIENTSDL_QUEUE_H
#define GAMECLIENTSDL_QUEUE_H

template <class T>
class SimpleQueue {

private:
    std::vector<T> v;
    SDL_mutex* mutex;
    SDL_cond* cond;
public:

    SimpleQueue() {
        this->mutex = SDL_CreateMutex();
        this->cond = SDL_CreateCond();
    }

    ~SimpleQueue() {
        delete this->mutex;
        delete this->cond;
    }

    void push(T item) {
        // Acquire the queue's lock
        SDL_LockMutex(this->mutex);
        // Allocate memory to store a new element of the queue
        this->v.push_back(item);
        // Send a signal that something has been pushed to the queue
        SDL_CondSignal(this->cond);
        // Release the lock on the queue
        SDL_UnlockMutex(this->mutex);
    }

    T pop() {
        T result;
        // Acquire the queue's lock
        SDL_LockMutex(this->mutex);
        // Queue is empty
        while (this->isEmpty()) {
            // Queue is empty -> wait for a signal that someone has pushed in the queue
            SDL_CondWait(this->cond, this->mutex);
        }
        // Get the most recent element added in the vector
        result = this->v.back();
        // Remove the last element from the vector
        this->v.pop_back();
        // Release the lock on the queue
        SDL_UnlockMutex(this->mutex);
        return result;
    }

    int size() {
        return this->v.size();
    }

    bool isEmpty() {
        return this->v.size() == 0;
    }
};

#endif //GAMECLIENTSDL_QUEUE_H
