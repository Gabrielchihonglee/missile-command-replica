#include "prep.h"
#include "functions.h"

#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

struct flashThreadArg {
    WINDOW *screen;
    int live;
    int x, y;
    char *text;
    int duration;
    int color_pair;
};

static WINDOW *flash_thread_screen;
static int flash_thread_live;
static int flash_thread_x, flash_thread_y;
static char *flash_thread_text;
static int flash_thread_duration;
static int flash_thread_color_pair;

void *flashFromString(void *arguments) {
    while (flash_thread_live) {
        pthread_mutex_lock(&lock);
        wattron(flash_thread_screen, COLOR_PAIR(flash_thread_color_pair));
        mvwprintw(flash_thread_screen, flash_thread_y, flash_thread_x, flash_thread_text);
        wrefresh(flash_thread_screen);
        pthread_mutex_unlock(&lock);
        if (!flash_thread_live) {
            break;
        }
        usleep(flash_thread_duration / 2);
        pthread_mutex_lock(&lock);
        wattron(flash_thread_screen, COLOR_PAIR(flash_thread_color_pair));
        for (int i = 0; i < strlen(flash_thread_text) - 1; i++) {
            mvwprintw(flash_thread_screen, flash_thread_y, flash_thread_x + i, " ");
            wrefresh(flash_thread_screen);
        }
        pthread_mutex_unlock(&lock);
        if (!flash_thread_live) {
            break;
        }
        usleep(flash_thread_duration / 2);
    }
    return NULL;
}

void prep() {
    WINDOW *prep_screen = newwin(FRAME_HEIGHT, FRAME_WIDTH, 0, 0);
    wattron(prep_screen, A_BOLD);
    noecho();
    wattron(prep_screen, COLOR_PAIR(84));
    drawFromFile(prep_screen, 0, FRAME_HEIGHT - 6, "graphics/ground", ERASE);
    int cities_x_pos[6] = {15, 30, 45, 70, 85, 100};
    for (int i = 0; i < 6; i++) {
        wattron(prep_screen, COLOR_PAIR(3));
        drawFromFile(prep_screen, cities_x_pos[i], FRAME_HEIGHT - 4, "graphics/city-layer-1", DRAW);
        wattron(prep_screen, COLOR_PAIR(5));
        drawFromFile(prep_screen, cities_x_pos[i], FRAME_HEIGHT - 4, "graphics/city-layer-2", DRAW);
    }
    wattron(prep_screen, COLOR_PAIR(5));
    drawFromFile(prep_screen, 18, FRAME_HEIGHT - 15, "graphics/defend-text", DRAW);
    drawFromFile(prep_screen, 72, FRAME_HEIGHT - 15, "graphics/cities-text", DRAW);

    int cur_score = 20;
    int high_score = 0;
    refreshHighScore(prep_screen, cur_score, high_score);

    wattron(prep_screen, COLOR_PAIR(2));
    char prep_screen_arrow_string[FRAME_WIDTH - 1];
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
    pthread_t prep_screen_arrow_thread;
    pthread_create(&prep_screen_arrow_thread, NULL, flashFromString, NULL);

    usleep(100000);

    carousel_thread_screen = prep_screen;
    carousel_thread_live = 1;
    carousel_thread_start_x = FRAME_WIDTH - 1;
    carousel_thread_end_x = 0;
    carousel_thread_y = FRAME_HEIGHT - 1;
    carousel_thread_text = "GABRIEL (LANC UNI ID: 37526367) @ 2019     INSERT COINS     1 COIN 1 PLAY";
    carousel_thread_color_pair = 84;
    pthread_t prep_screen_carousel_thread;
    pthread_create(&prep_screen_carousel_thread, NULL, carouselFromString, NULL);

    wgetch(prep_screen);
    werase(prep_screen);
    flash_thread_live = 0;
    carousel_thread_live = 0;
    pthread_join(prep_screen_arrow_thread, NULL);
    pthread_join(prep_screen_carousel_thread, NULL);
    delwin(prep_screen);
}
