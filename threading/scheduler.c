#include "../list.h"
#include "scheduler.h"

#include <ucontext.h>
#include <stdlib.h>

static struct list_item *thread_queue;

void sched_wakeup(struct thread *thread) { // add thread to queue
    struct list_item *thread_item = malloc(sizeof(*thread_item));
    *thread_item = (struct list_item){
        .content = thread,
        .next = NULL,
    };
    push_item_back(&thread_queue, thread_item);
}

void schedule() { // switch to the first thread in queue (pop off the first thread from the queue and schedule it)
    // TODO: call wait
    if (!thread_queue) {
        return;
    }
    // TODO: Fix memory leak
    swapcontext(&main_context, ((struct thread *)pop_item_front(&thread_queue)->content)->context);
}
