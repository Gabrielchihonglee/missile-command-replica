#include <ncurses.h>
#include <string.h>
#include <stdio.h>

int main() {
  initscr();
  raw();

  WINDOW * startScreen = newwin(20, 70, 0, 0);
  curs_set(FALSE);
  start_color();
  init_pair(1, COLOR_RED, COLOR_BLACK);
  wattron(startScreen, COLOR_PAIR(1));
  FILE *fp = fopen("graphics/missile-command-text", "r");
  char symbol;
  while ((symbol = getc(fp)) != EOF) {
    if (symbol == '#') {
      waddch(startScreen, ACS_CKBOARD);
    } else if (symbol == '\n') {
      waddch(startScreen, '\n');
    } else if (symbol == ' ') {
      waddch(startScreen, ' ');
    } else {
      perror("Unexpected character in missile-command-text");
    }
    wrefresh(startScreen);
  }
  fclose(fp);
  wattroff(startScreen, COLOR_PAIR(1));
  wgetch(startScreen);
  endwin();
}
