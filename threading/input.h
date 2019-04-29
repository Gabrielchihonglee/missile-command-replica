#ifndef INPUT_H
#define INPUT_H

#include "thread.h"
#include "scheduler.h"

#include <pthread.h>

pthread_mutex_t in_sleep;

void input_set_thread();

void *input_get();

void signal_dummy();

void input_init();

#endif
