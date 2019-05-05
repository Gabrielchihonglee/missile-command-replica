#ifndef THREAD_H
#define THREAD_H

#include <sys/mman.h>
#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>
#include <stdbool.h>

enum thread_state {STATE_RUNNING, STATE_IDLE, STATE_END};

struct thread {
    unsigned int id;
    ucontext_t context;
    enum thread_state state;
    bool wakeup;
    bool should_exit;
};

/**
*
*/
struct thread *thread_create(void (*fn)(void *param), void *param);

/**
*
*/
void *thead_wrapper(void (*fn)(void *param), void *param);

#endif
