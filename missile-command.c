#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#define FRAME_WIDTH 124
#define FRAME_HEIGHT 40

#define START_PADDING_HORIZONTAL 30
#define START_PADDING_VERTICAL 12

#define DRAW_MODE_ERASE 0
#define DRAW_MODE_FILL 1

#define START_SCREEN_EXPLODE_INNER_LOOP(num_from, num_to, stage_from, stage_to, color, wait) \
    wattron(start_screen, COLOR_PAIR(color)); \
    for (int i = num_from; i < num_to; i++) { \
        startScreenExplodeStage(start_screen, start_explosion_pos[i][0], start_explosion_pos[i][1], stage_from, stage_to); \
    } \
    usleep(wait);
#define START_SCREEN_EXPLODE_LOOP(num_from, num_to, color) \
    START_SCREEN_EXPLODE_INNER_LOOP(num_from, num_to, 0, 1, color, 50000); \
    START_SCREEN_EXPLODE_INNER_LOOP(num_from, num_to, 1, 2, color, 50000); \
    START_SCREEN_EXPLODE_INNER_LOOP(num_from, num_to, 2, 0, color, 1000);

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
                    //waddch(screen, '#');
                } else {
                    waddch(screen, ' ');
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

void startScreenTextColor(WINDOW *screen, int color) {
    wattron(screen, COLOR_PAIR(color));
    for (int i = START_PADDING_VERTICAL; i < (16 + START_PADDING_VERTICAL); i++) {
        for (int j = START_PADDING_HORIZONTAL; j < (63 + START_PADDING_HORIZONTAL); j++) {
            if ((mvwinch(screen, i, j) & A_CHARTEXT) == (ACS_CKBOARD & A_CHARTEXT)) {
                mvwaddch(screen, i, j, ACS_CKBOARD);
                wrefresh(screen);
            }
        }
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
    wattron(start_screen, COLOR_PAIR(2));
    drawFromFile(start_screen, START_PADDING_HORIZONTAL, START_PADDING_VERTICAL, "graphics/missile-command-text", 1);
    wrefresh(start_screen);
    usleep(1000000);

    int start_explosion_pos[80][2];
    for (int i = 0; i < 80; i++) {
        if (i % 3 == 0) {
            start_explosion_pos[i][0] = rand() % FRAME_WIDTH - 3;
            start_explosion_pos[i][1] = rand() % FRAME_HEIGHT - 3;
        } else {
            start_explosion_pos[i][0] = rand() % 63 + START_PADDING_HORIZONTAL - 2;
            start_explosion_pos[i][1] = rand() % 16 + START_PADDING_VERTICAL - 2;
        }
    }

    for (int i = 0; i < 2; i++) {
        START_SCREEN_EXPLODE_LOOP(0, 10, 4);
        startScreenTextColor(start_screen, 3);
        START_SCREEN_EXPLODE_LOOP(10, 20, 5);
        startScreenTextColor(start_screen, 4);
        START_SCREEN_EXPLODE_LOOP(20, 30, 6);
        startScreenTextColor(start_screen, 5);
        START_SCREEN_EXPLODE_LOOP(30, 40, 7);
        startScreenTextColor(start_screen, 6);
        START_SCREEN_EXPLODE_LOOP(40, 50, 8);
        startScreenTextColor(start_screen, 7);
        START_SCREEN_EXPLODE_LOOP(50, 60, 1);
        startScreenTextColor(start_screen, 8);
        START_SCREEN_EXPLODE_LOOP(60, 70, 2);
        startScreenTextColor(start_screen, 1);
        START_SCREEN_EXPLODE_LOOP(70, 80, 3);
        startScreenTextColor(start_screen, 2);
    }

    usleep(1000000);
    wattron(start_screen, COLOR_PAIR(4));
    mvwprintw(start_screen, FRAME_HEIGHT - 1, FRAME_WIDTH / 2 - strlen("PRESS ANY KEY TO CONTINUE") / 2, "PRESS ANY KEY TO CONTINUE");
    wgetch(start_screen);
    werase(start_screen);

    WINDOW *main_screen = newwin(FRAME_HEIGHT, FRAME_WIDTH, 0, 0);
    keypad(main_screen, TRUE);
    noecho();
    wmove(main_screen, 0, 0);

    wattron(main_screen, COLOR_PAIR(4));
    drawFromFile(main_screen, 0, FRAME_HEIGHT - 6, "graphics/ground", 1);
    wattron(main_screen, COLOR_PAIR(3));
    drawFromFile(main_screen, 15, FRAME_HEIGHT - 4, "graphics/city-layer-1", 1);
    wattron(main_screen, COLOR_PAIR(5));
    drawFromFile(main_screen, 15, FRAME_HEIGHT - 4, "graphics/city-layer-2", 1);
    wattron(main_screen, COLOR_PAIR(3));
    drawFromFile(main_screen, 30, FRAME_HEIGHT - 4, "graphics/city-layer-1", 1);
    wattron(main_screen, COLOR_PAIR(5));
    drawFromFile(main_screen, 30, FRAME_HEIGHT - 4, "graphics/city-layer-2", 1);
    wattron(main_screen, COLOR_PAIR(3));
    drawFromFile(main_screen, 45, FRAME_HEIGHT - 4, "graphics/city-layer-1", 1);
    wattron(main_screen, COLOR_PAIR(5));
    drawFromFile(main_screen, 45, FRAME_HEIGHT - 4, "graphics/city-layer-2", 1);
    wattron(main_screen, COLOR_PAIR(3));
    drawFromFile(main_screen, 70, FRAME_HEIGHT - 4, "graphics/city-layer-1", 1);
    wattron(main_screen, COLOR_PAIR(5));
    drawFromFile(main_screen, 70, FRAME_HEIGHT - 4, "graphics/city-layer-2", 1);
    wattron(main_screen, COLOR_PAIR(3));
    drawFromFile(main_screen, 85, FRAME_HEIGHT - 4, "graphics/city-layer-1", 1);
    wattron(main_screen, COLOR_PAIR(5));
    drawFromFile(main_screen, 85, FRAME_HEIGHT - 4, "graphics/city-layer-2", 1);
    wattron(main_screen, COLOR_PAIR(3));
    drawFromFile(main_screen, 100, FRAME_HEIGHT - 4, "graphics/city-layer-1", 1);
    wattron(main_screen, COLOR_PAIR(5));
    drawFromFile(main_screen, 100, FRAME_HEIGHT - 4, "graphics/city-layer-2", 1);
    //mvwaddch(main_screen, 10, 30, ACS_PLUS);

    int input;
    int cur_x = 30;
    int cur_y = 10;

    mvwaddch(main_screen, cur_y, cur_x, ACS_PLUS);

    while (1) {
        input = wgetch(main_screen);
        switch(input) {
            case KEY_LEFT:
                mvwaddch(main_screen, cur_y, cur_x, ' ');
                cur_x -= 1;
                mvwaddch(main_screen, cur_y, cur_x, ACS_PLUS);
                break;
            case KEY_RIGHT:
                mvwaddch(main_screen, cur_y, cur_x, ' ');
                cur_x += 1;
                mvwaddch(main_screen, cur_y, cur_x, ACS_PLUS);
                break;
            case KEY_UP:
                mvwaddch(main_screen, cur_y, cur_x, ' ');
                cur_y -= 1;
                mvwaddch(main_screen, cur_y, cur_x, ACS_PLUS);
                break;
            case KEY_DOWN:
                mvwaddch(main_screen, cur_y, cur_x, ' ');
                cur_y += 1;
                mvwaddch(main_screen, cur_y, cur_x, ACS_PLUS);
                break;
            case 'q':
                endwin();
                break;
        }
    }

    //wgetch(main_screen);

    endwin();
}
