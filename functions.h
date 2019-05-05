#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <ncurses.h>
#include <math.h>
#include <pthread.h>

#define FRAME_WIDTH 124
#define FRAME_HEIGHT 40

#define START_PADDING_HORIZONTAL 30
#define START_PADDING_VERTICAL 12

struct string_flash_arg {
    WINDOW *screen;
    int x, y;
    char *text;
    int duration;
    int color_pair;
};

struct carousel_arg {
    WINDOW *screen;
    int start_x, end_x, y;
    char *text;
    int color_pair;
};

extern pthread_mutex_t lock;

extern int start_explosion_pos[80][2];
extern char *STAGE_1, *STAGE_2, *LARGE_STAGE_1, *LARGE_STAGE_2;
extern int cities_x_pos[6];
extern int bases_x_pos[3];

enum drawMode {ERASE, DRAW};

extern int score;
extern int high_score;

void draw_from_file(WINDOW *screen, int start_x, int start_y, char file[], enum drawMode mode);

void draw_from_string(WINDOW *screen, int start_x, int start_y, char *line, enum drawMode mode);

char *file_to_string(char *file_name);

void update_small_explosion_stage(WINDOW *screen, int from_missile, int to_missile, int color);

void refresh_high_score(WINDOW *screen);

void carousel_from_string();

void draw_screen_settings(WINDOW *screen, int cities_only);

#endif
