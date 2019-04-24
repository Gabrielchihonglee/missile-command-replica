#include "list.h"
#include "thread.h"
#include "sleeper.h"
#include "scheduler.h"

#include <ucontext.h>
#include <stdlib.h>

static struct list_item *thread_queue;
struct thread *current_thread;

void sched_init() {
    struct thread *main_thread = malloc(sizeof(*main_thread));
    *main_thread = (struct thread) {
        .state = STATE_IDLE
    };
    current_thread = main_thread;
}

void sched_wakeup_no_check(struct thread *thread) {
    push_item_back(&thread_queue, thread);
}

void sched_wakeup(struct thread *thread) {
    if (thread != current_thread && !list_contains(&thread_queue, thread))
        sched_wakeup_no_check(thread);
}

void schedule() {
    if (current_thread->state == STATE_RUNNING)
        push_item_back(&thread_queue, current_thread);
    while (!thread_queue)
        sleep_wait();
    if ((struct thread *)thread_queue->content != current_thread) {
        struct thread *prev_thread = current_thread;
        current_thread = thread_queue->content;
        swapcontext(&prev_thread->context, &((struct thread *)pop_item_front(&thread_queue))->context);
    }
}
