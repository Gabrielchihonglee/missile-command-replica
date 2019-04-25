#ifndef LISTENER_H
#define LISTENER_H

#include "thread.h"
#include "scheduler.h"

#include <pthread.h>

extern pthread_mutex_t sched_queue_lock;

void listener();

void *listener_main();

void listener_init();

#endif
