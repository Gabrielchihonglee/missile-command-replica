#include "highscore.h"
#include "functions.h"

#include "threading/sleeper.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// draws the background with ground, cities and scores
void draw_highscore_background(WINDOW *screen) {
    for (int i = 0; i < 6; i++)
        cities[i].live = 1;
    draw_screen_settings(screen, 0, cities);
    refresh_high_score(screen);
}

void highscore() {
    WINDOW *score_screen = newwin(FRAME_HEIGHT, FRAME_WIDTH, 0, 0);
    wattron(score_screen, A_BOLD);
    wattron(score_screen, COLOR_PAIR(4));

    // read highscore file into entries
    FILE *highscore_file = fopen(".highscore", "r");
    char entry[12];
    char entries[8][2][6];
    if (highscore_file) {
        for (int i = 0; fgets(entry, sizeof(entry), highscore_file); i++)
            sscanf(entry, "%s %s", entries[i][0], entries[i][1]);
        fclose(highscore_file);
    }

    draw_highscore_background(score_screen);

    // get player initial
    mvwprintw(score_screen, 18, FRAME_WIDTH / 2 - 5, "GREAT SCORE");
    mvwprintw(score_screen, 20, FRAME_WIDTH / 2 - 9, "ENTER YOUR INITIALS");
    wmove(score_screen, 16, FRAME_WIDTH / 2 - 1);
    wrefresh(score_screen);
    char initial[4];
    for (int i = 0; i < 3; i++) {
        initial[i] = wgetch(score_screen);
        mvwaddch(score_screen, 16, FRAME_WIDTH / 2 - 1 + i, initial[i]);
        wrefresh(score_screen);
    }

    sleep_add(1, 0);

    werase(score_screen);
    draw_highscore_background(score_screen);
    mvwprintw(score_screen, 12, FRAME_WIDTH / 2 - 5, "HIGH SCORES");

    // add user record into table if it gets into the first 8

    // retermine the rank
    int rank = -1; // TODO: not a good way to handle the ranking, the whole part should be redo
    for (int i = 7; i > 0; i--) {
        if (atoi(entries[i][1]) < score)
            continue;
        rank = i;
        break;
    }

    // shift records to the end
    for (int i = 7; i > rank + 1; i--) {
        strcpy(entries[i][0], entries[i - 1][0]);
        strcpy(entries[i][1], entries[i - 1][1]);
    }

    // add new record
    strcpy(entries[rank + 1][0], initial);
    char score_str[6];
    sprintf(score_str, "%i", score);
    strcpy(entries[rank + 1][1], score_str);

    // display high score table
    wattron(score_screen, COLOR_PAIR(4));
    for (int i = 0; i < 8; i++) {
        mvwprintw(score_screen, i + 14, FRAME_WIDTH / 2 - 6, entries[i][0]);
        mvwprintw(score_screen, i + 14, FRAME_WIDTH / 2 + 1 + (6 - strlen(entries[i][1])), entries[i][1]);
    }
    wrefresh(score_screen);

    // write result back to highscore file
    highscore_file = fopen(".highscore", "w");
    if (highscore_file) {
        for (int i = 0; i < 8; i++)
            fprintf(highscore_file, "%s %s\n", entries[i][0], entries[i][1]);
        fclose(highscore_file);
    }

    wrefresh(score_screen);

    wgetch(score_screen);

    endwin();
    exit(0);
}
