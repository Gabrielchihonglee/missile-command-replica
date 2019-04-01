#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#define FRAME_WIDTH 125
#define FRAME_HEIGHT 45

#define START_PADDING_HORIZONTAL 30
#define START_PADDING_VERTICAL 12

#define DRAW_MODE_ERASE 0
#define DRAW_MODE_FILL 1

void drawFromFile(WINDOW *screen, int start_x, int start_y, char file[], int mode) {
    srand(time(0));
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
                if (mode == 1) {
                    waddch(screen, ACS_CKBOARD);
                } else {
                    waddch(screen, ' ');
                    //wdelch(screen);
                    //getyx(screen, y, x);
                    //wmove(screen, y, x + 1);
                }
                break;
            case '\n':
                getyx(screen, y, x);
                wmove(screen, y + 1, start_x);
                break;
            default:
                fprintf(stderr, "Unexpected character found in %s: '%c'", file, symbol);
        }
        wrefresh(screen);
        //nanosleep((const struct timespec[]){{0, 100000L}}, NULL);
    }
}

void startScreenExplodeStage(WINDOW *screen, int x, int y, int stage_from, int stage_to) {
    if (stage_from != 0) {
        char file_name_from[33];
        sprintf(file_name_from, "graphics/explosion-small-stage-%i", stage_from);
        drawFromFile(screen, x, y, file_name_from, 0);
        wrefresh(screen);
    }
    if (stage_to != 0) {
        char file_name_to[33];
        sprintf(file_name_to, "graphics/explosion-small-stage-%i", stage_to);
        drawFromFile(screen, x, y, file_name_to, 1);
        wrefresh(screen);
    }
}

int main() {
    initscr();
    raw();
    curs_set(0);

    start_color();
    //init_color(1, 255, 0, 0);
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);

    WINDOW *start_screen = newwin(FRAME_HEIGHT, FRAME_WIDTH, 0, 0);
    wattron(start_screen, COLOR_PAIR(1));
    drawFromFile(start_screen, START_PADDING_HORIZONTAL, START_PADDING_VERTICAL, "graphics/missile-command-text", 1);
    wrefresh(start_screen);
    wattroff(start_screen, COLOR_PAIR(1));
    usleep(500000);

    int start_explosion_pos[30][2];
    for (int i = 0; i < 15; i++) {
        start_explosion_pos[i][1] = rand() % 30 + 4; // x
        start_explosion_pos[i][0] = rand() % 110 + 4; // y
    }

    wattron(start_screen, COLOR_PAIR(2));

    for (int i = 0; i < 10; i++) {
        startScreenExplodeStage(start_screen, start_explosion_pos[i][0], start_explosion_pos[i][1], 0, 1);
    }
    usleep(500000);
    for (int i = 0; i < 10; i++) {
        startScreenExplodeStage(start_screen, start_explosion_pos[i][0], start_explosion_pos[i][1], 1, 2);
    }
    usleep(500000);
    for (int i = 0; i < 10; i++) {
        startScreenExplodeStage(start_screen, start_explosion_pos[i][0], start_explosion_pos[i][1], 2, 0);
    }
    for (int i = 10; i < 20; i++) {
        startScreenExplodeStage(start_screen, start_explosion_pos[i][0], start_explosion_pos[i][1], 0, 1);
    }
    usleep(500000);
    for (int i = 10; i < 20; i++) {
        startScreenExplodeStage(start_screen, start_explosion_pos[i][0], start_explosion_pos[i][1], 1, 2);
    }
    usleep(500000);
    for (int i = 10; i < 20; i++) {
        startScreenExplodeStage(start_screen, start_explosion_pos[i][0], start_explosion_pos[i][1], 2, 0);
    }
    for (int i = 20; i < 30; i++) {
        startScreenExplodeStage(start_screen, start_explosion_pos[i][0], start_explosion_pos[i][1], 0, 1);
    }
    usleep(500000);
    for (int i = 20; i < 30; i++) {
        startScreenExplodeStage(start_screen, start_explosion_pos[i][0], start_explosion_pos[i][1], 1, 2);
    }
    usleep(500000);
    for (int i = 20; i < 30; i++) {
        startScreenExplodeStage(start_screen, start_explosion_pos[i][0], start_explosion_pos[i][1], 2, 0);
    }

    wgetch(start_screen);
    wattroff(start_screen, COLOR_PAIR(2));

    endwin();
}
