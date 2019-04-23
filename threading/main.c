#include "list.h"
#include "thread.h"
#include "scheduler.h"
#include "sleeper.h"

#include <ucontext.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

void print_1(void *param) { // demo function
    printf("1\n");
}

void print_2(void *param) { // demo function
    printf("2\n");
}

void print_3(void *param) { // demo function
    printf("3\n");
}

int main() {
    sched_init();

    struct thread thread_1 = thread_create(&print_1, NULL);
    struct thread thread_2 = thread_create(&print_2, NULL);
    struct thread thread_3 = thread_create(&print_3, NULL);

    sched_wakeup(&thread_1);
    sched_wakeup(&thread_2);
    sched_wakeup(&thread_3);

    schedule();

    printf("END!!!\n");

    return 0;
}
