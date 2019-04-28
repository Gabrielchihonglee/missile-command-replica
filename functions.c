#include "functions.h"

#include "threading/sleeper.h"

#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

struct carouselThreadArg {
    WINDOW *screen;
    int live;
    int start_x, end_x, y;
    char *text;
    int color_pair;
};

pthread_mutex_t lock;

int start_explosion_pos[80][2];
char *STAGE_1, *STAGE_2, *LARGE_STAGE_1, *LARGE_STAGE_2;
int cities_x_pos[6] = {15, 30, 45, 70, 85, 100};
int bases_x_pos[3] = {1, 56, 112};

//enum drawMode {ERASE, DRAW};

WINDOW *carousel_thread_screen;
int carousel_thread_live;
int carousel_thread_start_x, carousel_thread_end_x, carousel_thread_y;
char *carousel_thread_text;
int carousel_thread_color_pair;

int score = 0;
int high_score = 0;

void draw_from_file(WINDOW *screen, int start_x, int start_y, char file[], enum drawMode mode) { // mode 1: draw 0: erase/draw with backgound
    FILE *fp = fopen(file, "r");
    char symbol;
    int x, y;
    wmove(screen, start_y, start_x);
    while ((symbol = getc(fp)) != EOF) {
        switch (symbol) {
            case '.':
                getyx(screen, y, x);
                wmove(screen, y, x + 1);
                break;
            case '#':
                if (mode == 1)
                    waddch(screen, ACS_CKBOARD);
                else
                    waddch(screen, ' ');
                break;
            case '\n':
                getyx(screen, y, x);
                wmove(screen, y + 1, start_x);
                break;
            default:
                fprintf(stderr, "Unexpected character found in %s: '%c'", file, symbol);
                break;
        }
        wrefresh(screen);
    }
}

void draw_from_string(WINDOW *screen, int start_x, int start_y, char *line, enum drawMode mode) {
    int length = strlen(line);
    int x, y;
    wmove(screen, start_y, start_x);
    for (int i = 0; i < length; i++) {
        switch (line[i]) {
            case '.':
                getyx(screen, y, x);
                wmove(screen, y, x + 1);
                break;
            case '#':
                if (mode == 1)
                    waddch(screen, ACS_CKBOARD);
                else
                    waddch(screen, ' ');
                break;
            case '\n':
                getyx(screen, y, x);
                wmove(screen, y + 1, start_x);
                break;
            default:
                fprintf(stderr, "Unexpected character found: '%i'", line[i]);
                break;
        }
        wrefresh(screen);
    }
}

char *file_to_string(char *file_name) {
    FILE *file = fopen(file_name, "r");
    fseek(file, 0, SEEK_END);
    int length = ftell(file);
    char *string = malloc(length + 1);
    fseek(file, 0, SEEK_SET);
    fread(string, 1, length, file);
    string[length] = '\0';
    fclose(file);
    return string;
}

void update_small_explosion_stage(WINDOW *screen, int from_missile, int to_missile, int color) {
    wattron(screen, COLOR_PAIR(color));
    for (int i = from_missile; i < to_missile; i++)
        draw_from_string(screen, start_explosion_pos[i][0], start_explosion_pos[i][1], STAGE_1, DRAW); // draw stage 1
    sleep_add(0, 60000000);

    wattron(screen, COLOR_PAIR(color));
    for (int i = from_missile; i < to_missile; i++)
        draw_from_string(screen, start_explosion_pos[i][0], start_explosion_pos[i][1], STAGE_2, DRAW); // draw stage 2
    sleep_add(0, 60000000);

    wattron(screen, COLOR_PAIR(color));
    for (int i = from_missile; i < to_missile; i++)
        draw_from_string(screen, start_explosion_pos[i][0], start_explosion_pos[i][1], STAGE_2, ERASE); // erase stage 2
    sleep_add(0, 60000000);
}

/**
SCORE CALCULATION METHOD:
Base Scores:
    Missile: 25
    Crazy Missile: 125
    UFO / Fighter: 100
    Unused missile: 5/missile
    Unused city: 100/cities
Score Multiplier:
    Level 1/2: 1x
    Level 3/4: 2x
    Level 5/6: 3x
    Level 7/8: 4x
    Level 8/9: 5x
    Level 11+: 6x
Info from: https://strategywiki.org/wiki/Missile_Command/Walkthrough
**/

void refresh_high_score(WINDOW *screen) {
    char score_text[10];
    char high_score_text[10];
    sprintf(score_text, "%i", score);
    sprintf(high_score_text, "%i", high_score);
    wattron(screen, COLOR_PAIR(2));
    mvwprintw(screen, 0, FRAME_WIDTH / 2, high_score_text);
    mvwprintw(screen, 0, FRAME_WIDTH / 2 - 15, score_text);
}

void carousel_from_string() {
    wrefresh(carousel_thread_screen);
    int carousel_thread_head_x = carousel_thread_start_x;
    while (carousel_thread_live) {
        pthread_mutex_lock(&lock);
        for (int i = 0; i < (FRAME_WIDTH - 1 - carousel_thread_head_x) && i < strlen(carousel_thread_text); i++) {
            if ((carousel_thread_head_x + i) < carousel_thread_end_x)
                continue;
            wattron(carousel_thread_screen, COLOR_PAIR(carousel_thread_color_pair));
            mvwaddch(carousel_thread_screen, carousel_thread_y, carousel_thread_head_x + i, carousel_thread_text[i]);
        }
        if ((carousel_thread_head_x + strlen(carousel_thread_text)) < (FRAME_WIDTH - 1)) {
            wattron(carousel_thread_screen, COLOR_PAIR(carousel_thread_color_pair));
            mvwaddch(carousel_thread_screen, carousel_thread_y, carousel_thread_head_x + strlen(carousel_thread_text), ' ');
        }
        pthread_mutex_unlock(&lock);
        wrefresh(carousel_thread_screen);
        sleep_add(0, 100000000);
        carousel_thread_head_x--;
        if (carousel_thread_head_x + strlen(carousel_thread_text) + 1 <= carousel_thread_end_x)
            carousel_thread_head_x = carousel_thread_start_x;
    }
}

void draw_screen_settings(WINDOW *screen, int cities_only) {
    pthread_mutex_lock(&lock);
    if (!cities_only) {
        wattron(screen, COLOR_PAIR(84));
        draw_from_file(screen, 0, FRAME_HEIGHT - 3, "graphics/ground", ERASE);
        wattron(screen, COLOR_PAIR(84));
        for (int i = 0; i < 3; i++)
            draw_from_file(screen, bases_x_pos[i], FRAME_HEIGHT - 6, "graphics/base", ERASE);
    }
    for (int i = 0; i < 6; i++) {
        if (cities_x_pos[i] == -1)
            continue;
        wattron(screen, COLOR_PAIR(3));
        draw_from_file(screen, cities_x_pos[i], FRAME_HEIGHT - 4, "graphics/city-layer-1", DRAW);
        wattron(screen, COLOR_PAIR(5));
        draw_from_file(screen, cities_x_pos[i], FRAME_HEIGHT - 4, "graphics/city-layer-2", DRAW);
    }
    pthread_mutex_unlock(&lock);
}
