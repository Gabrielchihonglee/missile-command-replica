#include "../list.h"
#include "scheduler.h"
#include "sleeper.h"

#include <ucontext.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define STACK_SIZE 1048576

ucontext_t main_context;

enum thread_state {STATE_RUNNABLE, STATE_SLEEP};

struct thread {
    unsigned int id;
    ucontext_t *context;
    enum thread_state state;
};

static struct list_item *threads;

struct list_item *pop_item_front(struct list_item **list) {
    struct list_item *ret = *list;
    if (ret)
        *list = ret->next;
    return ret;
}

unsigned int gen_thread_id() {
    static unsigned int id = 0;
    id++;
    return id;
}

// seems to be unused, pending deletion
struct thread *add_thread(ucontext_t *context) { // add thread to thread list
    struct thread *thread = malloc(sizeof(*thread));
    *thread = (struct thread){
        .id = gen_thread_id(),
        .context = context,
        .state = STATE_RUNNABLE
    };
    struct list_item *thread_item = malloc(sizeof(*thread_item));
    *thread_item = (struct list_item){
        .content = thread,
        .next = NULL,
    };
    push_item_back(&threads, thread_item);
    //push_item_order_sleep(&sleeping_threads, thread_item);
    return thread;
}

void print_1(ucontext_t *context) { // demo function
    printf("1\n");
    swapcontext(context, &main_context);
}

void print_2(ucontext_t *context) { // demo function
    printf("2\n");
    swapcontext(context, &main_context);
}

void print_3(ucontext_t *context) { // demo function
    printf("3\n");
    swapcontext(context, &main_context);
}

int main() {
    void *stack[5];
    stack_t stack_struct[5];

    struct timespec spec;

    for (int i = 0; i < 5; i++) {
        stack[i] = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (stack[i] == MAP_FAILED) {
            perror("mmap");
            exit(1);
        }
        stack_struct[i] = (stack_t) {
            .ss_sp = stack[i],
            .ss_size = STACK_SIZE
        };
    }

    //for (int i = 0; i < 5; i++) {
        ucontext_t *context = malloc(sizeof(*context));
        getcontext(context);
        context->uc_stack = stack_struct[0];
        makecontext(context, (void *)&print_1, 1, context);
        struct thread *thread_1 = malloc(sizeof(*thread_1));
        clock_gettime(CLOCK_REALTIME, &spec);
        *thread_1 = (struct thread){
            .id = gen_thread_id(),
            .context = context,
            .state = STATE_RUNNABLE
        };
        sleep_add(thread_1, spec.tv_sec + 1);

        context = malloc(sizeof(*context));
        getcontext(context);
        context->uc_stack = stack_struct[1];
        makecontext(context, (void *)&print_2, 1, context);
        struct thread *thread_2 = malloc(sizeof(*thread_2));
        clock_gettime(CLOCK_REALTIME, &spec);
        *thread_2 = (struct thread){
            .id = gen_thread_id(),
            .context = context,
            .state = STATE_RUNNABLE
        };
        sleep_add(thread_2, spec.tv_sec + 2);

        context = malloc(sizeof(*context));
        getcontext(context);
        context->uc_stack = stack_struct[2];
        makecontext(context, (void *)&print_3, 1, context);
        struct thread *thread_3 = malloc(sizeof(*thread_3));
        clock_gettime(CLOCK_REALTIME, &spec);
        *thread_3 = (struct thread){
            .id = gen_thread_id(),
            .context = context,
            .state = STATE_RUNNABLE
        };
        sleep_add(thread_3, spec.tv_sec + 3);
    //}

    while(1) {
        wakeup_threads();
        schedule();
        //struct timespec spec;
        clock_gettime(CLOCK_REALTIME, &spec);
    }

    printf("END!!!\n");

    return 0;
}
