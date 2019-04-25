#ifndef SCHEDUER_H
#define SCHEDUER_H

#include "list.h"
#include "thread.h"
#include "sleeper.h"

#include <ucontext.h>
#include <stdlib.h>

struct thread;
extern struct thread *current_thread;

void sched_init();

void sched_wakeup_no_check(struct thread *thread);

/**
* Adds the thread to the queue.
*/
void sched_wakeup(struct thread *thread);

/**
* Switches to the first thread in queue (pop off the first thread from the queue and schedule it)
* Also add thread back to queue if it isn't dead..
*/
void schedule();

#endif
