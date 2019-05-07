#include "highscore.h"
#include "functions.h"

#include "threading/list.h"
#include "threading/thread.h"
#include "threading/input.h"
#include "threading/scheduler.h"
#include "threading/sleeper.h"

#include <ncurses.h>
#include <stdlib.h>

void highscore() {
    WINDOW *score_screen = newwin(FRAME_HEIGHT, FRAME_WIDTH, 0, 0);
    wattron(score_screen, A_BOLD);
    wattron(score_screen, COLOR_PAIR(4));

    for (int i = 0; i < 6; i++)
        cities[i].live = 1;
    draw_screen_settings(score_screen, 0, cities);
    refresh_high_score(score_screen);
    wrefresh(score_screen);

    sleep_add(1, 0);

    endwin();
    exit(0);
}
