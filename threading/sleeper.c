#include "../list.h"
#include "sleeper.h"

#include <time.h>

static struct list_item *sleeping_threads;

int sleep_sort(void *a, void *b) {
    struct thread *b_t = b;
    struct thread *a_t = a;
    return a_t->wakeup - b_t->wakeup;
}

void sleep_add(struct thread *thread, long wakeup) {
    struct sleeping_thread sleeping_thread = {
        .thead = thread,
        .wakeup = wakeup
    };
    push_item_order(&sleeping_threads, &sleeping_thread, &sleep_sort);
}

void sleep_wait() {
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    if (sleeping_threads) {
        if (((struct thread *) sleeping_threads->content)->wakeup <= spec.tv_sec) {
            sched_wakeup((struct thread *) pop_item_front(&sleeping_threads)->thread);
        }
    }
}
