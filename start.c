#include "start.h"
#include "prep.h"
#include "functions.h"

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

void start_screen_text_color(WINDOW *screen, int color) {
    wattron(screen, COLOR_PAIR(color));
    for (int i = START_PADDING_VERTICAL; i < (16 + START_PADDING_VERTICAL); i++)
        for (int j = START_PADDING_HORIZONTAL; j < (63 + START_PADDING_HORIZONTAL); j++)
            if ((mvwinch(screen, i, j) & A_CHARTEXT) == (ACS_CKBOARD & A_CHARTEXT)) {
                mvwaddch(screen, i, j, ACS_CKBOARD);
                wrefresh(screen);
            }
}

void start() {
    WINDOW *start_screen = newwin(FRAME_HEIGHT, FRAME_WIDTH, 0, 0);
    wattron(start_screen, A_BOLD);
    wattron(start_screen, COLOR_PAIR(2));
    draw_from_file(start_screen, START_PADDING_HORIZONTAL, START_PADDING_VERTICAL, "graphics/missile-command-text", DRAW);
    wrefresh(start_screen);
    sleep_add(1, 0);

    for (int i = 0; i < 80; i++) {
        if (i % 3 == 0) {
            start_explosion_pos[i][0] = rand() % (FRAME_WIDTH - 3);
            start_explosion_pos[i][1] = rand() % (FRAME_HEIGHT - 3);
        } else {
            start_explosion_pos[i][0] = rand() % 63 + START_PADDING_HORIZONTAL - 2;
            start_explosion_pos[i][1] = rand() % 16 + START_PADDING_VERTICAL - 2;
        }
    }

    STAGE_1 = file_to_string("graphics/explosion-small-stage-1");
    STAGE_2 = file_to_string("graphics/explosion-small-stage-2");

    for (int i = 0; i < 3; i++) {
        update_small_explosion_stage(start_screen, 0, 10, 4);
        start_screen_text_color(start_screen, 3);
        update_small_explosion_stage(start_screen, 10, 20, 5);
        start_screen_text_color(start_screen, 4);
        update_small_explosion_stage(start_screen, 20, 30, 6);
        start_screen_text_color(start_screen, 5);
        update_small_explosion_stage(start_screen, 30, 40, 7);
        start_screen_text_color(start_screen, 6);
        update_small_explosion_stage(start_screen, 40, 50, 8);
        start_screen_text_color(start_screen, 7);
        update_small_explosion_stage(start_screen, 50, 60, 1);
        start_screen_text_color(start_screen, 8);
        update_small_explosion_stage(start_screen, 60, 70, 2);
        start_screen_text_color(start_screen, 1);
        update_small_explosion_stage(start_screen, 70, 80, 3);
        start_screen_text_color(start_screen, 2);
    }

    sleep_add(1, 0);
    wattron(start_screen, COLOR_PAIR(8));
    mvwprintw(start_screen, FRAME_HEIGHT - 1, FRAME_WIDTH / 2 - strlen("PRESS ANY KEY TO CONTINUE") / 2, "PRESS ANY KEY TO CONTINUE");
    wgetch(start_screen);
    werase(start_screen);
    delwin(start_screen);

    struct thread *prep_thread = thread_create(&prep, NULL);
    sched_wakeup(prep_thread);
}
