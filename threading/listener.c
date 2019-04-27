#define _GNU_SOURCE

#include "listener.h"
#include "thread.h"
#include "scheduler.h"

#include <pthread.h>

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <signal.h>

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>

#include <termios.h>

struct thread *input_thread;
pthread_mutex_t stop_read_input;

sigset_t mask;
sigset_t old_mask;

void input_set_thread() {
    input_thread = current_thread;
    input_thread->state = STATE_IDLE;
    schedule();
}

void handle_input() {
    while (1) {
        input_set_thread();
        char input;
        read(1, &input, 1);
        printf("input: %c\n", input);
    }
    // do whatever is needed if input is detected
}

void *input_get(void *argument) {
    while (1)
        if (input_thread) {
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(0, &rfds);
            select(1, &rfds, NULL, NULL, NULL);

            sched_wakeup(input_thread);
            pthread_mutex_lock(&in_sleep);
            syscall(SYS_tgkill, getpid(), getpid(), SIGUSR1);
            input_thread = NULL;
            pthread_mutex_unlock(&in_sleep);
        }
}

void input_init() {
    pthread_mutex_init(&stop_read_input, NULL);

    pthread_t input_get_pthread;
    pthread_create(&input_get_pthread, NULL, input_get, NULL);

    // disable ICANON to get live input
    struct termios orig_tios;
    tcgetattr(0, &orig_tios);
    struct termios tios = orig_tios;
    tios.c_lflag &= ~ICANON;
    tcsetattr(0, TCSANOW, &tios);

    struct thread *input_handler = thread_create(&handle_input, NULL);
    sched_wakeup(input_handler);
}
