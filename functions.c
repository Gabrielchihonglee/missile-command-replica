#include "functions.h"

#include "threading/sleeper.h"

#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

int start_explosion_pos[80][2];
char *STAGE_1, *STAGE_2, *LARGE_STAGE_1, *LARGE_STAGE_2;
int cities_x_pos[6] = {15, 30, 45, 70, 85, 100};
int bases_x_pos[3] = {1, 56, 112};

int score = 0;
int high_score = 0;

void draw_fill(WINDOW *screen) {
    for (int i = 0; i < FRAME_HEIGHT; i++)
        for (int j = 0; j < FRAME_WIDTH; j++)
            mvwaddch(screen, i, j, ACS_CKBOARD);
}

void draw_from_file(WINDOW *screen, int start_x, int start_y, char file[], enum draw_mode mode) {
    FILE *fp = fopen(file, "r");
    char symbol;
    int x, y;
    wmove(screen, start_y, start_x);
    while ((symbol = getc(fp)) != EOF) { // draw char by char
        switch (symbol) {
            case '.': // blank
                getyx(screen, y, x);
                wmove(screen, y, x + 1);
                break;
            case '#': // fill
                if (mode == 1)
                    waddch(screen, ACS_CKBOARD);
                else
                    waddch(screen, ' ');
                break;
            case '\n': // new line
                getyx(screen, y, x);
                wmove(screen, y + 1, start_x);
                break;
            default: // ?!
                if (mode == 1)
                    waddch(screen, symbol);
                else
                    waddch(screen, ' ');
                //fprintf(stderr, "Unexpected character found in %s: '%c'", file, symbol);
                break;
        }
        wrefresh(screen);
    }
}

void draw_from_string(WINDOW *screen, int start_x, int start_y, char *line, enum draw_mode mode) {
    int length = strlen(line);
    int x, y;
    wmove(screen, start_y, start_x);
    for (int i = 0; i < length; i++) {
        switch (line[i]) {
            case '.': // blank
                getyx(screen, y, x);
                wmove(screen, y, x + 1);
                break;
            case '#': // fill
                if (mode == 1)
                    waddch(screen, ACS_CKBOARD);
                else
                    waddch(screen, ' ');
                break;
            case '\n': // new line
                getyx(screen, y, x);
                wmove(screen, y + 1, start_x);
                break;
            default: // ?!
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
    char *string = malloc(length + 1); // leaving space for '\0'
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

void flash_from_string(void *args) {
    struct string_flash_arg *string_flash_arg = args;
    while (!current_thread->should_exit) {
        // print text
        wattron(string_flash_arg->screen, COLOR_PAIR(string_flash_arg->color_pair));
        mvwprintw(string_flash_arg->screen, string_flash_arg->y, string_flash_arg->x, string_flash_arg->text);
        wrefresh(string_flash_arg->screen);

        if (current_thread->should_exit)
            break;
        sleep_add(0, string_flash_arg->duration / 2 * 1000);

        // hide text
        wattron(string_flash_arg->screen, COLOR_PAIR(string_flash_arg->color_pair));
        for (int i = 0; i < strlen(string_flash_arg->text) - 1; i++) {
            mvwprintw(string_flash_arg->screen, string_flash_arg->y, string_flash_arg->x + i, " ");
            wrefresh(string_flash_arg->screen);
        }

        if (current_thread->should_exit)
            break;
        sleep_add(0, string_flash_arg->duration / 2 * 1000);
    }
    free(string_flash_arg);
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
    Level 9/10: 5x
    Level 11+: 6x
Info from: https://strategywiki.org/wiki/Missile_Command/Walkthrough
**/

int score_multiplier(int score, int level) {
    switch (level) {
        case 1:
        case 2:
            return score;
        case 3:
        case 4:
            return score * 2;
        case 5:
        case 6:
            return score * 3;
        case 7:
        case 8:
            return score * 4;
        case 9:
        case 10:
            return score * 5;
        default:
            return score * 6;
    }
}

void refresh_high_score(WINDOW *screen) {
    char score_text[10];
    char high_score_text[10];
    sprintf(score_text, "%i", score);
    sprintf(high_score_text, "%i", high_score);
    wattron(screen, COLOR_PAIR(2));
    mvwprintw(screen, 0, FRAME_WIDTH / 2, high_score_text);
    mvwprintw(screen, 0, FRAME_WIDTH / 2 - 15, score_text);
}

void carousel_from_string(void *arg) {
    struct carousel_arg *carousel_arg = arg;
    wrefresh(carousel_arg->screen);
    int head_x = carousel_arg->start_x; // keep record of the position of the first character on the screen
    while (!current_thread->should_exit) {
        for (int i = 0; i < (FRAME_WIDTH - 1 - head_x) && i < strlen(carousel_arg->text); i++) { // print the string char by char
            if ((head_x + i) < carousel_arg->end_x) // skip if the char is out of the printable range
                continue;
            wattron(carousel_arg->screen, COLOR_PAIR(carousel_arg->color_pair));
            mvwaddch(carousel_arg->screen, carousel_arg->y, head_x + i, carousel_arg->text[i]);
        }
        if ((head_x + strlen(carousel_arg->text)) < (FRAME_WIDTH - 1)) { // see if the end of the string is on screen
            wattron(carousel_arg->screen, COLOR_PAIR(carousel_arg->color_pair));
            mvwaddch(carousel_arg->screen, carousel_arg->y, head_x + strlen(carousel_arg->text), ' '); // remove the last char from the screen
        }
        wrefresh(carousel_arg->screen);
        sleep_add(0, 100000000);
        head_x--; // move the string to left
        if (head_x + strlen(carousel_arg->text) + 1 <= carousel_arg->end_x) // if whole string is out of string
            head_x = carousel_arg->start_x; // reset
    }
}

void draw_screen_settings(WINDOW *screen, int cities_only, struct city cities[6]) {
    if (!cities_only) { // draw ground and base if not only to draw the cities
        wattron(screen, COLOR_PAIR(84));
        draw_from_file(screen, 0, FRAME_HEIGHT - 3, "graphics/ground", ERASE);
        wattron(screen, COLOR_PAIR(84));
        for (int i = 0; i < 3; i++)
            draw_from_file(screen, bases_x_pos[i], FRAME_HEIGHT - 6, "graphics/base", ERASE);
    }
    for (int i = 0; i < 6; i++) {
        if (!cities[i].live) // skips all cities that aren't live
            continue;
        wattron(screen, COLOR_PAIR(3));
        draw_from_file(screen, cities_x_pos[i], FRAME_HEIGHT - 4, "graphics/city-layer-1", DRAW);
        wattron(screen, COLOR_PAIR(5));
        draw_from_file(screen, cities_x_pos[i], FRAME_HEIGHT - 4, "graphics/city-layer-2", DRAW);
    }
}
