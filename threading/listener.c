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

struct thread *listener_thread;

void listener() {
    current_thread->state = STATE_RUNNING;
    char input;
    read(1, &input, 1);
    printf("\n\n\n\n\n");
    printf("input: %c", input);
    current_thread->state = STATE_END;
    schedule();
    // do whatever is needed if input is detected
}

void *listener_main(void *argument) {
    while (1) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        select(1, &rfds, NULL, NULL, NULL);
        syscall(SYS_tgkill, getpid(), getpid(), SIGUSR1);
        pthread_mutex_lock(&sched_queue_lock);
        sched_wakeup_no_check(listener_thread);
        pthread_mutex_unlock(&sched_queue_lock);
    }
}

void listener_init() {
    pthread_t listener_main_thread;
    pthread_create(&listener_main_thread, NULL, listener_main, NULL);

    struct termios orig_tios;
    tcgetattr(0, &orig_tios);
    struct termios tios = orig_tios;
    tios.c_lflag &= ~ICANON;
    tcsetattr(0, TCSANOW, &tios);

    listener_thread = thread_create(&listener, NULL);
    listener_thread->state = STATE_IDLE;
}
