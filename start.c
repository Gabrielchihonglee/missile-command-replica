#include "start.h"
#include "functions.h"
#include "prep.h"

#include "threading/thread.h"
#include "threading/input.h"
#include "threading/scheduler.h"
#include "threading/sleeper.h"

#include <ncurses.h>
#include <string.h>

// changes the color of the text "MISSILE COMMAND"
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

    // setup explosion position
    for (int i = 0; i < 80; i++) {
        if (!i % 3) { // 1 in 3 will explode without boundaries
            start_explosion_pos[i][0] = rand() % (FRAME_WIDTH - 3);
            start_explosion_pos[i][1] = rand() % (FRAME_HEIGHT - 3);
        } else { // 2 in 3 will explode on the "MISSILE COMMAND" text
            start_explosion_pos[i][0] = rand() % 63 + START_PADDING_HORIZONTAL - 2;
            start_explosion_pos[i][1] = rand() % 16 + START_PADDING_VERTICAL - 2;
        }
    }

    // setup the "shape" for the explosions
    STAGE_1 = file_to_string("graphics/explosion-small-stage-1");
    STAGE_2 = file_to_string("graphics/explosion-small-stage-2");

    // start explosion
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 8; j++) {
            update_small_explosion_stage(start_screen, j * 10, j * 10 + 10, rand() % 8); // explodes 10 at a time
            start_screen_text_color(start_screen, rand() % 8);
        }

    sleep_add(1, 0);
    wattron(start_screen, COLOR_PAIR(8));
    mvwprintw(start_screen, FRAME_HEIGHT - 1, FRAME_WIDTH / 2 - strlen("PRESS ANY KEY TO CONTINUE") / 2, "PRESS ANY KEY TO CONTINUE");
    wgetch(start_screen);
    werase(start_screen);
    delwin(start_screen);

    // run stage prep
    sched_wakeup(thread_create(&prep, NULL));
}
