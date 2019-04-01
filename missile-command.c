#include <ncurses.h>
#include <string.h>
#include <stdio.h>

int main() {
  initscr();
  raw();

  WINDOW * startScreen = newwin(45, 135, 0, 0);
  curs_set(FALSE);
  start_color();
  init_pair(1, COLOR_RED, COLOR_WHITE);
  wattron(startScreen, COLOR_PAIR(1));
  FILE *fp = fopen("graphics/missile-command-text", "r");
  char symbol;
  int y, x;
  for (int i = 0; i < 12; i++) {
    getyx(startScreen, y, x);
    mvwaddch(startScreen, y, 123, ACS_VLINE);
    wmove(startScreen, y + 1, 0);
  }
  getyx(startScreen, y, x);
  wmove(startScreen, y, x + 30);
  while ((symbol = getc(fp)) != EOF) {
    if (symbol == '#') {
      waddch(startScreen, ACS_CKBOARD);
    } else if (symbol == '\n') {
      getyx(startScreen, y, x);
      //wmove(startScreen, y, 103);
      mvwaddch(startScreen, y, 123, ACS_VLINE);
      wrefresh(startScreen);
      getyx(startScreen, y, x);
      wmove(startScreen, y + 1, 30);
      //wprintw(startScreen, "          ");
    } else if (symbol == ' ') {
      getyx(startScreen, y, x);
      wmove(startScreen, y, x + 1);
    } else {
      perror("Unexpected character in missile-command-text");
    }
    wrefresh(startScreen);
  }
  for (int i = 0; i < 12; i++) {
    getyx(startScreen, y, x);
    mvwaddch(startScreen, y, 123, ACS_VLINE);
    wmove(startScreen, y + 1, 0);
  }
  for (int i = 0; i < 123; i++) {
    waddch(startScreen, ACS_HLINE);
    wrefresh(startScreen);
  }
  waddch(startScreen, ACS_LRCORNER);
  wrefresh(startScreen);
  fclose(fp);
  wattroff(startScreen, COLOR_PAIR(1));
  wgetch(startScreen);
  endwin();
}
