#ifndef LISTENER_H
#define LISTENER_H

#include "thread.h"
#include "scheduler.h"

#include <pthread.h>

pthread_mutex_t listener_lock;

void listener();

void *listener_main();

void listener_init();

#endif
