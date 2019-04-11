#include "start.h"
#include "functions.h"

#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

void startScreenTextColor(WINDOW *screen, int color) {
    wattron(screen, COLOR_PAIR(color));
    pthread_mutex_lock(&lock);
    for (int i = START_PADDING_VERTICAL; i < (16 + START_PADDING_VERTICAL); i++) {
        for (int j = START_PADDING_HORIZONTAL; j < (63 + START_PADDING_HORIZONTAL); j++) {
            if ((mvwinch(screen, i, j) & A_CHARTEXT) == (ACS_CKBOARD & A_CHARTEXT)) {
                mvwaddch(screen, i, j, ACS_CKBOARD);
                wrefresh(screen);
            }
        }
    }
    pthread_mutex_unlock(&lock);
}

void start() {
    WINDOW *start_screen = newwin(FRAME_HEIGHT, FRAME_WIDTH, 0, 0);
    wattron(start_screen, A_BOLD);
    wattron(start_screen, COLOR_PAIR(2));
    drawFromFile(start_screen, START_PADDING_HORIZONTAL, START_PADDING_VERTICAL, "graphics/missile-command-text", DRAW);
    wrefresh(start_screen);
    usleep(1000000);

    for (int i = 0; i < 80; i++) {
        if (i % 3 == 0) {
            start_explosion_pos[i][0] = rand() % (FRAME_WIDTH - 3);
            start_explosion_pos[i][1] = rand() % (FRAME_HEIGHT - 3);
        } else {
            start_explosion_pos[i][0] = rand() % 63 + START_PADDING_HORIZONTAL - 2;
            start_explosion_pos[i][1] = rand() % 16 + START_PADDING_VERTICAL - 2;
        }
    }

    FILE *explosion_small_stage_1 = fopen("graphics/explosion-small-stage-1", "r");
    fseek (explosion_small_stage_1, 0, SEEK_END);
    int stage_1_length = ftell(explosion_small_stage_1);
    STAGE_1 = malloc(sizeof(char) * stage_1_length + 1);
    fseek (explosion_small_stage_1, 0, SEEK_SET);
    fread(STAGE_1, 1, stage_1_length, explosion_small_stage_1);
    STAGE_1[stage_1_length] = '\0';
    fclose(explosion_small_stage_1);

    FILE *explosion_small_stage_2 = fopen("graphics/explosion-small-stage-2", "r");
    fseek (explosion_small_stage_2, 0, SEEK_END);
    int stage_2_length = ftell(explosion_small_stage_2);
    STAGE_2 = malloc(sizeof(char) * stage_2_length + 1);
    fseek (explosion_small_stage_2, 0, SEEK_SET);
    fread(STAGE_2, 2, stage_2_length, explosion_small_stage_2);
    STAGE_2[stage_2_length] = '\0';
    fclose(explosion_small_stage_2);

    for (int i = 0; i < 2; i++) {
        updateSmallExplosionStage(start_screen, 0, 10, 4);
        startScreenTextColor(start_screen, 3);
        updateSmallExplosionStage(start_screen, 10, 20, 5);
        startScreenTextColor(start_screen, 4);
        updateSmallExplosionStage(start_screen, 20, 30, 6);
        startScreenTextColor(start_screen, 5);
        updateSmallExplosionStage(start_screen, 30, 40, 7);
        startScreenTextColor(start_screen, 6);
        updateSmallExplosionStage(start_screen, 40, 50, 8);
        startScreenTextColor(start_screen, 7);
        updateSmallExplosionStage(start_screen, 50, 60, 1);
        startScreenTextColor(start_screen, 8);
        updateSmallExplosionStage(start_screen, 60, 70, 2);
        startScreenTextColor(start_screen, 1);
        updateSmallExplosionStage(start_screen, 70, 80, 3);
        startScreenTextColor(start_screen, 2);
    }

    usleep(1000000);
    wattron(start_screen, COLOR_PAIR(84));
    mvwprintw(start_screen, FRAME_HEIGHT - 1, FRAME_WIDTH / 2 - strlen("PRESS ANY KEY TO CONTINUE") / 2, "PRESS ANY KEY TO CONTINUE");
    wgetch(start_screen);
    werase(start_screen);
    delwin(start_screen);
}
