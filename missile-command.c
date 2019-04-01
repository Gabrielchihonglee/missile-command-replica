#include <ncurses.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define FRAME_WIDTH 125
#define FRAME_HEIGHT 45

#define START_PADDING_HORIZONTAL 30
#define START_PADDING_VERTICAL 12

void drawFromFile(WINDOW *screen, int start_x, int start_y, char file[]) {
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
                waddch(screen, ACS_CKBOARD);
                break;
            case '\n':
                getyx(screen, y, x);
                wmove(screen, y + 1, start_x);
                break;
            default:
                fprintf(stderr, "Unexpected character found in %s: '%c'", file, symbol);
        }
        wrefresh(screen);
        nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);
    }
}

int main() {
    initscr();
    raw();
    curs_set(0);

    init_pair(1, COLOR_RED, COLOR_WHITE);

    WINDOW *start_screen = newwin(FRAME_HEIGHT, FRAME_WIDTH, 0, 0);
    wattron(start_screen, COLOR_PAIR(1));
    drawFromFile(start_screen, START_PADDING_HORIZONTAL, START_PADDING_VERTICAL, "graphics/missile-command-text");
    wgetch(start_screen);
    wattroff(start_screen, COLOR_PAIR(1));

    endwin();
}
