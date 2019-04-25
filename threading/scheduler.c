#include "list.h"
#include "thread.h"
#include "sleeper.h"
#include "scheduler.h"

#include <pthread.h>

#include <ucontext.h>
#include <stdlib.h>

static struct list_item *thread_queue;
struct thread *current_thread;
struct thread *main_thread;

pthread_mutex_t sched_queue_lock;

void sched_init() {
    pthread_mutex_init(&sched_queue_lock, NULL);

    main_thread = malloc(sizeof(*main_thread));
    *main_thread = (struct thread) {
        .state = STATE_IDLE
    };
    current_thread = main_thread;
}

void sched_wakeup_no_check(struct thread *thread) {
    pthread_mutex_lock(&sched_queue_lock);
    push_item_back(&thread_queue, thread);
    pthread_mutex_unlock(&sched_queue_lock);
}

void sched_wakeup(struct thread *thread) {
    if (thread != current_thread && !list_contains(&thread_queue, thread))
        sched_wakeup_no_check(thread);
}

void swap_to(struct thread *thread) {
    if (thread == current_thread)
        return;
    struct thread *prev = current_thread;
    current_thread = thread;
    swapcontext(&prev->context, &current_thread->context);
}

void schedule() {
    if (current_thread != main_thread && current_thread->state == STATE_RUNNING) {
        pthread_mutex_lock(&sched_queue_lock);
        push_item_back(&thread_queue, current_thread);
        pthread_mutex_unlock(&sched_queue_lock);
    }
    if (!thread_queue)
        swap_to(main_thread);
    else
        swap_to(pop_item_front(&thread_queue));
}
