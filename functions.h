#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <ncurses.h>

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

struct city {
    int x, y;
    int live;
} cities[6];

extern int start_explosion_pos[80][2];
extern char *STAGE_1, *STAGE_2, *LARGE_STAGE_1, *LARGE_STAGE_2;
extern int cities_x_pos[6];
extern int bases_x_pos[3];

enum draw_mode {ERASE, DRAW};

extern int score;
extern int high_score;

/**
* Fills the screen
*/
void draw_fill(WINDOW *screen);

/**
* Draws file's contents on screen
*/
void draw_from_file(WINDOW *screen, int start_x, int start_y, char file[], enum draw_mode mode);

/**
* Draws string on screen
*/
void draw_from_string(WINDOW *screen, int start_x, int start_y, char *line, enum draw_mode mode);

/**
* Returns the file's contents as a string
*/
char *file_to_string(char *file_name);

/**
* Displays the "animation" for small explosion by drawing and erasing stages of the explosion
*/
void update_small_explosion_stage(WINDOW *screen, int from_missile, int to_missile, int color);

/**
* Displays string periodically, giving the effect of "flshing"
*/
void flash_from_string(void *args);

/**
* Gives the score multiplied according to the spec and current level
*/
int score_multiplier(int score, int level);

/**
* Updates the current score and highscore displayed on screen
*/
void refresh_high_score(WINDOW *screen);

/**
* Displays a carousel test
*/
void carousel_from_string();

/**
* Draws the setting of the screen, including the ground, bases and cities.
*/
void draw_screen_settings(WINDOW *screen, int cities_only, struct city cities[6]);

#endif
