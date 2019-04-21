#ifndef SCHEDUER_H
#define SCHEDUER_H

#include "../list.h"

#include <ucontext.h>
#include <stdlib.h>

struct thread;

void sched_wakeup(struct thread *thread);

void schedule();

#endif
