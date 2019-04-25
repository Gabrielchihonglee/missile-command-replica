#ifndef LISTENER_H
#define LISTENER_H

#include "thread.h"
#include "scheduler.h"

#include <pthread.h>

void listener();

void *listener_main();

void listener_init();

#endif
