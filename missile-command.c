#include <ncurses.h>
#include <string.h>
#include <stdio.h>

#define FRAME_WIDTH 125
#define FRAME_HEIGHT 45

#define TITLE_PADDING_HORIZONTAL 30
#define TITLE_PADDING_VERTICAL 12

int main() {
    initscr();
    raw();

    WINDOW *start_screen = newwin(FRAME_HEIGHT, FRAME_WIDTH, 0, 0);
    curs_set(FALSE);
    start_color();
    init_pair(1, COLOR_RED, COLOR_WHITE);
    wattron(start_screen, COLOR_PAIR(1));
    FILE *fp = fopen("graphics/missile-command-text", "r");
    char symbol;
    int y, x;
    for (int i = 0; i < TITLE_PADDING_VERTICAL; i++) {
        getyx(start_screen, y, x);
        mvwaddch(start_screen, y, FRAME_WIDTH - 2, ACS_VLINE);
        wmove(start_screen, y + 1, 0);
    }
    getyx(start_screen, y, x);
    wmove(start_screen, y, x + TITLE_PADDING_HORIZONTAL);
    while ((symbol = getc(fp)) != EOF) {
        if (symbol == '#') {
            waddch(start_screen, ACS_CKBOARD);
        } else if (symbol == '\n') {
            getyx(start_screen, y, x);
            mvwaddch(start_screen, y, FRAME_WIDTH - 2, ACS_VLINE);
            wrefresh(start_screen);
            getyx(start_screen, y, x);
            wmove(start_screen, y + 1, TITLE_PADDING_HORIZONTAL);
        } else if (symbol == ' ') {
            getyx(start_screen, y, x);
            wmove(start_screen, y, x + 1);
        } else {
            perror("Unexpected character in missile-command-text");
        }
        wrefresh(start_screen);
    }
    for (int i = 0; i < TITLE_PADDING_VERTICAL; i++) {
        getyx(start_screen, y, x);
        mvwaddch(start_screen, y, FRAME_WIDTH - 2, ACS_VLINE);
        wmove(start_screen, y + 1, 0);
    }
    for (int i = 0; i < FRAME_WIDTH - 2; i++) {
        waddch(start_screen, ACS_HLINE);
        wrefresh(start_screen);
    }
    waddch(start_screen, ACS_LRCORNER);
    wrefresh(start_screen);
    fclose(fp);
    wattroff(start_screen, COLOR_PAIR(1));
    wgetch(start_screen);
    endwin();
}
