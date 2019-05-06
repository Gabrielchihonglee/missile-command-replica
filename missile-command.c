#include "functions.h"
#include "start.h"
#include "prep.h"
#include "game.h"
#include "end.h"

#include "threading/list.h"
#include "threading/thread.h"
#include "threading/input.h"
#include "threading/scheduler.h"
#include "threading/sleeper.h"

#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>

int main() {
    pthread_mutex_init(&in_sleep, NULL);
    sched_init();
    input_init();
    signal(SIGUSR1, signal_dummy);

    srand(time(0));
    initscr();
    raw();
    curs_set(0);

    pthread_mutex_init(&lock, NULL);

    start_color();
    init_pair(1, COLOR_BLACK, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    init_pair(5, COLOR_BLUE, COLOR_BLACK);
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(7, COLOR_CYAN, COLOR_BLACK);
    init_pair(8, COLOR_WHITE, COLOR_BLACK);

    init_pair(48, COLOR_YELLOW, COLOR_WHITE);
    init_pair(84, COLOR_WHITE, COLOR_YELLOW);

    struct thread *start_thread = thread_create(&end, NULL);
    sched_wakeup(start_thread);

    //prep();

    //game();

    while(1) {
        schedule();
        sleep_till_next();
        // something like "errno != EINTR" here
    }

    endwin();
    pthread_mutex_destroy(&lock);
}
