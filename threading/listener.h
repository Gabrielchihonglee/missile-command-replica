#ifndef LISTENER_H
#define LISTENER_H

#include "thread.h"
#include "scheduler.h"

#include <pthread.h>

pthread_mutex_t in_sleep;

void *input_get();

void input_init();

#endif
