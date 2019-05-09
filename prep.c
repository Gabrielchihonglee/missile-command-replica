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

// stores state of threads created by prep() for input handler's use
struct input_arg {
    struct thread *string_flash_thread;
    struct thread *carousel_thread;
};

// the input handler for prep screen
void prep_screen_input(void *arg) {
    struct input_arg *input_arg = arg;
    int input;
    input_set_thread();
    input = wgetch(prep_screen);

    // end all threads upon input
    input_arg->string_flash_thread->should_exit = true;
    input_arg->carousel_thread->should_exit = true;

    free(arg);

    switch (input) {
        case 'q':
            endwin();
            exit(0);
            break;
        default:
            sched_wakeup(thread_create(&game, NULL));
    }
}

void prep() {
    prep_screen = newwin(FRAME_HEIGHT, FRAME_WIDTH, 0, 0);
    wattron(prep_screen, A_BOLD);
    noecho();

    // set all cities to live so they get printed
    for (int i = 0; i < 6; i++)
        cities[i].live = 1;
    draw_screen_settings(prep_screen, 0, cities);

    wattron(prep_screen, COLOR_PAIR(5));
    draw_from_file(prep_screen, 18, FRAME_HEIGHT - 15, "graphics/defend-text", DRAW);
    draw_from_file(prep_screen, 72, FRAME_HEIGHT - 15, "graphics/cities-text", DRAW);

    refresh_high_score(prep_screen);

    // create the string for the flashing arrows
    char *prep_screen_arrow_string = malloc(sizeof(char) * (FRAME_WIDTH - 1));
    for (int i = 0; i < FRAME_WIDTH - 1; i++)
        prep_screen_arrow_string[i] = ' ';
    for (int i = 0; i < 6; i++)
        prep_screen_arrow_string[cities_x_pos[i] + 3] = 'V';
    prep_screen_arrow_string[FRAME_WIDTH - 1] = '\0';

    // flashing arrows above the cities
    struct string_flash_arg *string_flash_arg = malloc(sizeof(*string_flash_arg));
    *string_flash_arg = (struct string_flash_arg) {
        .screen = prep_screen,
        .x = 0,
        .y = FRAME_HEIGHT - 7,
        .text = prep_screen_arrow_string,
        .duration = 1200000,
        .color_pair = 2
    };
    struct thread *prep_screen_arrow_thread = thread_create(&flash_from_string, string_flash_arg);
    sched_wakeup(prep_screen_arrow_thread);

    sleep_add(1, 0);

    // carousel text at the botom
    struct carousel_arg *carousel_arg = malloc(sizeof(*carousel_arg));
    *carousel_arg = (struct carousel_arg) {
        .screen = prep_screen,
        .start_x = FRAME_WIDTH - 1,
        .end_x = 0,
        .y = FRAME_HEIGHT - 1,
        .text = "GABRIEL (LANC UNI ID: 37526367) @ 2019     GRADE A+ PLEASE?     INSERT COINS     1 COIN 1 PLAY",
        .color_pair = 84
    };
    struct thread *prep_screen_carousel_thread = thread_create(&carousel_from_string, carousel_arg);
    sched_wakeup(prep_screen_carousel_thread);

    // input handler
    struct input_arg *input_arg = malloc(sizeof(*input_arg));
    *input_arg = (struct input_arg) {
        .string_flash_thread = prep_screen_arrow_thread,
        .carousel_thread = prep_screen_carousel_thread
    };
    sched_wakeup(thread_create(&prep_screen_input, input_arg));
}
