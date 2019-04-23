#ifndef THREAD_H
#define THREAD_H

#include <sys/mman.h>
#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>

enum thread_state {STATE_RUNNING, STATE_IDLE, STATE_END};

struct thread {
    int id;
    ucontext_t context;
    enum thread_state state;
};

/**
*
*/
struct thread *thread_create(void (*fn)(void *param), void *param, int id);

/**
*
*/
void *thead_wrapper(void (*fn)(void *param), void *param);

#endif
