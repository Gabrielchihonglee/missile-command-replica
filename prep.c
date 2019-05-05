#include "prep.h"
#include "game.h"
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
#include <signal.h>

WINDOW *prep_screen;

struct string_flash_arg {
    WINDOW *screen;
    int live;
    int x, y;
    char *text;
    int duration;
    int color_pair;
} *string_flash_arg;

static WINDOW *flash_thread_screen;
static int flash_thread_live;
static int flash_thread_x, flash_thread_y;
static char *flash_thread_text;
static int flash_thread_duration;
static int flash_thread_color_pair;

struct thread *game_thread;

void flash_from_string() {
    while (flash_thread_live) {
        wattron(flash_thread_screen, COLOR_PAIR(flash_thread_color_pair));
        mvwprintw(flash_thread_screen, flash_thread_y, flash_thread_x, flash_thread_text);
        wrefresh(flash_thread_screen);
        if (!flash_thread_live)
            break;
        sleep_add(0, flash_thread_duration / 2 * 1000);
        wattron(flash_thread_screen, COLOR_PAIR(flash_thread_color_pair));
        for (int i = 0; i < strlen(flash_thread_text) - 1; i++) {
            mvwprintw(flash_thread_screen, flash_thread_y, flash_thread_x + i, " ");
            wrefresh(flash_thread_screen);
        }
        if (!flash_thread_live)
            break;
        sleep_add(0, flash_thread_duration / 2 * 1000);
    }
}

void prep_screen_input() {
    int input;
    input_set_thread();
    input = wgetch(prep_screen);
    flash_thread_live = 0;
    free(string_flash_arg);
    carousel_thread_live = 0;
    switch (input) {
        case 'q':
            endwin();
            exit(0);
            break;
        default:
            sched_wakeup(game_thread);
    }
}

void prep() {
    struct thread *input_handler = thread_create(&prep_screen_input, NULL);
    sched_wakeup(input_handler);

    prep_screen = newwin(FRAME_HEIGHT, FRAME_WIDTH, 0, 0);
    wattron(prep_screen, A_BOLD);
    noecho();
    draw_screen_settings(prep_screen, 0);
    wattron(prep_screen, COLOR_PAIR(5));
    draw_from_file(prep_screen, 18, FRAME_HEIGHT - 15, "graphics/defend-text", DRAW);
    draw_from_file(prep_screen, 72, FRAME_HEIGHT - 15, "graphics/cities-text", DRAW);

    refresh_high_score(prep_screen);

    wattron(prep_screen, COLOR_PAIR(2));
    char *prep_screen_arrow_string = malloc(sizeof(char) * (FRAME_WIDTH - 1));
    for (int i = 0; i < FRAME_WIDTH - 1; i++) {
        prep_screen_arrow_string[i] = ' ';
    }
    for (int i = 0; i < 6; i++) {
        prep_screen_arrow_string[cities_x_pos[i] + 3] = 'V';
    }
    prep_screen_arrow_string[FRAME_WIDTH - 1] = '\0';
    flash_thread_screen = prep_screen;
    flash_thread_live = 1;
    flash_thread_x = 0;
    flash_thread_y = FRAME_HEIGHT - 7;
    flash_thread_text = prep_screen_arrow_string;
    flash_thread_duration = 1200000;
    flash_thread_color_pair = 2;
    struct thread *prep_screen_arrow_thread = thread_create(&flash_from_string, NULL);
    sched_wakeup(prep_screen_arrow_thread);

    sleep_add(1, 0);

    carousel_thread_screen = prep_screen;
    carousel_thread_live = 1;
    carousel_thread_start_x = FRAME_WIDTH - 1;
    carousel_thread_end_x = 0;
    carousel_thread_y = FRAME_HEIGHT - 1;
    carousel_thread_text = "GABRIEL (LANC UNI ID: 37526367) @ 2019     INSERT COINS     1 COIN 1 PLAY";
    carousel_thread_color_pair = 84;
    struct thread *prep_screen_carousel_thread = thread_create(&carousel_from_string, NULL);
    sched_wakeup(prep_screen_carousel_thread);

    game_thread = thread_create(&game, NULL);
}
