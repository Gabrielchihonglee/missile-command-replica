#ifndef SLEEPER_H
#define SLEEPER_H

#include "../list.h"

#include <time.h>

struct sleeping_thread {
    struct thread *thread;
    long wakeup;
};

/**
* Adds sleeping_thread to the sleeping_threads list, ordered by wakeup time.
*/
void sleep_add(struct thread *thread, long wakeup);

/**
* Checks if the first sleeping_thread should wake up, if so, calls sched_wakeup,
* also removing the sleeping_thread from the sleeping_threads list.
*/
void wakeup_threads();

#endif
