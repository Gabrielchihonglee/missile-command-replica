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

#define START_SCREEN_EXPLODE_INNER_LOOP(num_from, num_to, stage_from, stage_to, wait) \
    for (int i = num_from; i < num_to; i++) { \
        startScreenExplodeStage(start_screen, start_explosion_pos[i][0], start_explosion_pos[i][1], stage_from, stage_to); \
    } \
    usleep(wait);
#define START_SCREEN_EXPLODE_LOOP(num_from, num_to) \
    START_SCREEN_EXPLODE_INNER_LOOP(num_from, num_to, 0, 1, 30000); \
    START_SCREEN_EXPLODE_INNER_LOOP(num_from, num_to, 1, 2, 30000); \
    START_SCREEN_EXPLODE_INNER_LOOP(num_from, num_to, 2, 0, 0);

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
    init_pair(1, COLOR_BLACK, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    init_pair(5, COLOR_BLUE, COLOR_BLACK);
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(7, COLOR_CYAN, COLOR_BLACK);
    init_pair(8, COLOR_WHITE, COLOR_BLACK);

    WINDOW *start_screen = newwin(FRAME_HEIGHT, FRAME_WIDTH, 0, 0);
    wattron(start_screen, COLOR_PAIR(8));
    drawFromFile(start_screen, START_PADDING_HORIZONTAL, START_PADDING_VERTICAL, "graphics/missile-command-text", 1);
    wrefresh(start_screen);
    wattroff(start_screen, COLOR_PAIR(8));
    usleep(500000);

    int start_explosion_pos[80][2];
    for (int i = 0; i < 80; i++) {
        if (i % 3 == 0) {
            start_explosion_pos[i][0] = rand() % 119 + 1;
            start_explosion_pos[i][1] = rand() % 39 + 1;
        } else {
            start_explosion_pos[i][0] = rand() % 60 + 28;
            start_explosion_pos[i][1] = rand() % 16 + 10;
        }
    }

    for (int i = 0; i < 2; i++) {
        wattron(start_screen, COLOR_PAIR(4));
        START_SCREEN_EXPLODE_LOOP(0, 10);
        wattron(start_screen, COLOR_PAIR(5));
        START_SCREEN_EXPLODE_LOOP(10, 20);
        wattron(start_screen, COLOR_PAIR(6));
        START_SCREEN_EXPLODE_LOOP(20, 30);
        wattron(start_screen, COLOR_PAIR(7));
        START_SCREEN_EXPLODE_LOOP(30, 40);
        wattron(start_screen, COLOR_PAIR(8));
        START_SCREEN_EXPLODE_LOOP(40, 50);
        wattron(start_screen, COLOR_PAIR(1));
        START_SCREEN_EXPLODE_LOOP(50, 60);
        wattron(start_screen, COLOR_PAIR(2));
        START_SCREEN_EXPLODE_LOOP(60, 70);
        wattron(start_screen, COLOR_PAIR(3));
        START_SCREEN_EXPLODE_LOOP(70, 80);
    }

/**
    char temp[10];
    char test = mvwinch(start_screen, 12, 30) & A_CHARTEXT;
    mvwaddch(start_screen, 0, 0, test);
    sprintf(temp, "%lu", ACS_CKBOARD);
    wprintw(start_screen, temp);

    if ((mvwinch(start_screen, 12, 30) & A_CHARTEXT) == ACS_CKBOARD) {
        endwin();
    }
**/
    wgetch(start_screen);
    wattroff(start_screen, COLOR_PAIR(3));

    endwin();
}
