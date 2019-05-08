#include "functions.h"
#include "start.h"
#include "game.h"

#include "threading/thread.h"
#include "threading/input.h"
#include "threading/scheduler.h"

#include <ncurses.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>

int main() {
    pthread_mutex_init(&in_sleep, NULL);
    sched_init();
    input_init();
    signal(SIGUSR1, signal_dummy);

    srand(time(0));
    //srand(1912);
    initscr();
    raw();
    curs_set(0);

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

    // get highscore
    FILE *highscore_file = fopen(".highscore", "r");
    char entry[13];
    char temp[6];
    char high_score_text[6];
    if (highscore_file) {
        fgets(entry, sizeof(entry), highscore_file);
        sscanf(entry, "%s %s", temp, high_score_text);
        fclose(highscore_file);
    }
    high_score = atoi(high_score_text);

    // run stage start
    struct thread *start_thread = thread_create(&game, NULL);
    sched_wakeup(start_thread);

    // main loop for the scheduler
    while(1) {
        schedule();
        sleep_till_next();
    }

    endwin();
}
