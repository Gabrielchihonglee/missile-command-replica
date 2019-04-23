#include "list.h"
#include "thread.h"
#include "scheduler.h"
#include "sleeper.h"

#include <time.h>

static struct list_item *sleeping_threads;

int sleep_sort(void *a, void *b) {
    struct timespec a_time = ((struct sleeping_thread *)a)->wakeup;
    struct timespec b_time = ((struct sleeping_thread *)b)->wakeup;
    if (a_time.tv_sec == b_time.tv_sec)
        return (long)a_time.tv_nsec - b_time.tv_nsec;
    return (long)a_time.tv_sec - b_time.tv_sec;
}

void sleep_add(struct thread *thread, struct timespec wakeup) {
    struct sleeping_thread *sleeping_thread = malloc(sizeof(*sleeping_thread));
    *sleeping_thread = (struct sleeping_thread){
        .thread = thread,
        .wakeup = wakeup
    };
    push_item_order(&sleeping_threads, sleeping_thread, &sleep_sort);

    thread->state = STATE_IDLE;
    schedule();
    thread->state = STATE_RUNNING;
}

void sleep_wait() {
    if (sleeping_threads) {
        struct timespec spec;
        clock_gettime(CLOCK_REALTIME, &spec);
        struct timespec wakeup_time = ((struct sleeping_thread *)sleeping_threads->content)->wakeup;
        if (wakeup_time.tv_sec > spec.tv_sec || (wakeup_time.tv_sec == spec.tv_sec && wakeup_time.tv_nsec > spec.tv_nsec)) {
            if (wakeup_time.tv_nsec < spec.tv_nsec) {
                wakeup_time.tv_sec -= 1;
                wakeup_time.tv_nsec += 1000000000;
            }
            struct timespec sleep_length = {
                .tv_sec = wakeup_time.tv_sec - spec.tv_sec,
                .tv_nsec = wakeup_time.tv_nsec - spec.tv_nsec
            };
            nanosleep(&sleep_length, NULL);
            clock_gettime(CLOCK_REALTIME, &spec);
        }

        while (
            sleeping_threads &&
            ((wakeup_time = ((struct sleeping_thread *)sleeping_threads->content)->wakeup),
            (wakeup_time.tv_sec < spec.tv_sec || (wakeup_time.tv_sec == spec.tv_sec && wakeup_time.tv_nsec <= spec.tv_nsec)))
        )  {
            struct sleeping_thread *item = pop_item_front(&sleeping_threads);
            sched_wakeup(item->thread);
            free(item);
        }
    }
    // sleep forever till interrupt
    schedule();
}
