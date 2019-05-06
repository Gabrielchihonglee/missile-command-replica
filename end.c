#include "functions.h"

#include "threading/list.h"
#include "threading/thread.h"
#include "threading/input.h"
#include "threading/scheduler.h"
#include "threading/sleeper.h"

#include <ncurses.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>

int radius = 30;
int center_x = FRAME_WIDTH/2 - 1;
int center_y = FRAME_HEIGHT/2 - 1;

void print_vertex(WINDOW *screen, int num) {
    mvwaddch(screen, round(sin(M_PI/4 * num) * radius * 0.5 + center_y), round(cos(M_PI/4 * num) * radius + center_x), 'X');
    char temp[20];
    sprintf(temp, "%i,%i", (int) round(cos(M_PI/4 * num) * radius + center_x), (int) round(sin(M_PI/4 * num) * radius * 0.5 + center_y));
    mvwprintw(screen, (int) round(sin(M_PI/4 * num) * radius * 0.5 + center_y) - 1, (int) round(cos(M_PI/4 * num) * radius + center_x) - 2, temp);
    //sprintf(tempp, "%f, %f", (cos(M_PI/4 * num) * radius + center_x), (sin(M_PI/4 * num) * radius + center_y));
    //mvwprintw(screen, (sin(M_PI/4 * num) * radius + center_y - 2), (cos(M_PI/4 * num) * radius + center_x - 9), tempp);
    wrefresh(screen);
    sleep_add(0, 100000000);
}

void print_edge(WINDOW *screen, int num_from, int num_to) {
    int from_x = round(cos(M_PI/4 * num_from) * radius + center_x);
    int from_y = round(sin(M_PI/4 * num_from) * radius * 0.5 + center_y);
    int to_x = round(cos(M_PI/4 * num_to) * radius + center_x);
    int to_y = round(sin(M_PI/4 * num_to) * radius * 0.5 + center_y);

    if (from_x > to_x) {
        int temp = from_x;
        from_x = to_x;
        to_x = temp;
        temp = from_y;
        from_y = to_y;
        to_y = temp;
    }

    for (int i = from_x; i < to_x; i++) {
        mvwaddch(screen, round((double)(to_y - from_y) / (to_x - from_x) * (i - from_x) + from_y), i, '#');
    }

    wrefresh(screen);
    sleep_add(0, 100000000);
}

bool should_fill(int x, int y) {
    double angle;
    if (x == center_x) {
        if (y > center_y)
            angle = M_PI / 2 * 3;
        else
            angle = M_PI / 2;
    } else {
        angle = atan((double)(center_y - y) * 2 / (x - center_x));
        if (x < center_x)
            angle += M_PI;
        if (x > center_x && y >= center_y)
            angle += M_PI * 2;
    }
    angle = 2 * M_PI - angle;
    int region = angle/(M_PI/4);

    int from_x = round(cos(M_PI/4 * region) * radius + center_x);
    int from_y = round(sin(M_PI/4 * region) * radius * 0.5 + center_y);
    int to_x = round(cos(M_PI/4 * (region + 1)) * radius + center_x);
    int to_y = round(sin(M_PI/4 * (region + 1)) * radius * 0.5 + center_y);

    //return region + 1;

    double boundary_y = (double) (to_y - from_y) / (to_x - from_x) * (x - from_x) + from_y;
    if (y > boundary_y && y > center_y) // point is lower than line && lower half
        return true;
    if (y < boundary_y && y < center_y) // point is higher than line && upper half/horizontal middle
        return true;
    if (y == center_y) {
        if (y > boundary_y && x > center_x)
            return true;
        if (y < boundary_y && x < center_x)
            return true;
    }
    return false;
}

void fill_out(WINDOW *screen) {
    for (int i = 0; i < FRAME_HEIGHT; i++)
        for (int j = 0; j < FRAME_WIDTH; j+=1)
            if (should_fill(j, i))
                mvwaddch(screen, i, j, ACS_CKBOARD);
}

void end() {
    WINDOW *end_screen = newwin(FRAME_HEIGHT, FRAME_WIDTH, 0, 0);
    wattron(end_screen, A_BOLD);
    wattron(end_screen, COLOR_PAIR(4));

    char *the_end_text = file_to_string("graphics/the-end-text");

    for (int i = 2; i < (FRAME_HEIGHT - 2); i++) {
        werase(end_screen);
        wattron(end_screen, COLOR_PAIR(rand() % 8 + 1));
        draw_fill(end_screen);
        wattron(end_screen, COLOR_PAIR(2));
        draw_from_string(end_screen, FRAME_WIDTH / 2 - 28, FRAME_HEIGHT / 2 - 4, the_end_text, DRAW);
        radius = i;
        fill_out(end_screen);
        wrefresh(end_screen);
        usleep(80000);
        //sleep_add(0, 80000000);
    }

    for (int i = (FRAME_HEIGHT - 2); i > 2; i--) {
        werase(end_screen);
        wattron(end_screen, COLOR_PAIR(rand() % 8 + 1));
        draw_fill(end_screen);
        wattron(end_screen, COLOR_PAIR(2));
        draw_from_string(end_screen, FRAME_WIDTH / 2 - 28, FRAME_HEIGHT / 2 - 4, the_end_text, DRAW);
        wattron(end_screen, COLOR_PAIR(2));
        radius = i;
        fill_out(end_screen);
        wrefresh(end_screen);
        sleep_add(0, 80000000);
    }

    endwin();
    exit(0);
}
