#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#define FRAME_WIDTH 124
#define FRAME_HEIGHT 40

#define START_PADDING_HORIZONTAL 30
#define START_PADDING_VERTICAL 12
/**
#define SCORE_MISSILE 25
#define SCORE_FIGHTER 50
#define SCORE_UFO 60
**/
#define START_SCREEN_EXPLODE_INNER_LOOP(num_from, num_to, string_from, string_to, color, wait) \
    wattron(start_screen, COLOR_PAIR(color)); \
    for (int i = num_from; i < num_to; i++) { \
        startScreenExplodeStage(start_screen, start_explosion_pos[i][0], start_explosion_pos[i][1], string_from, string_to); \
    } \
    usleep(wait);

#define START_SCREEN_EXPLODE_LOOP(num_from, num_to, color) \
    START_SCREEN_EXPLODE_INNER_LOOP(num_from, num_to, "", stage_1, color, 100000); \
    START_SCREEN_EXPLODE_INNER_LOOP(num_from, num_to, stage_1, stage_2, color, 100000); \
    START_SCREEN_EXPLODE_INNER_LOOP(num_from, num_to, stage_2, "", color, 1000);

struct missile {
    int x, y;
    int vel_x, vel_y;
};

void drawFromFile(WINDOW *screen, int start_x, int start_y, char file[], int mode) {
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
                break;
        }
        wrefresh(screen);
        //nanosleep((const struct timespec[]){{0, 100000L}}, NULL);
    }
}

void drawFromFileExact(WINDOW *screen, int start_x, int start_y, char file[], int mode) {
    FILE *fp = fopen(file, "r");
    char symbol;
    int x, y;
    wmove(screen, start_y, start_x);
    while ((symbol = getc(fp)) != EOF) {
        //waddch(screen, symbol);
        switch (symbol) {
            case '\n':
                getyx(screen, y, x);
                wmove(screen, y + 1, start_x);
                break;
            default:
                waddch(screen, symbol);
                break;
        }
        wrefresh(screen);
        //nanosleep((const struct timespec[]){{0, 100000L}}, NULL);
    }
}

void drawFromString(WINDOW *screen, int start_x, int start_y, char *line, int mode) {
    int length = strlen(line);
    int x, y;
    wmove(screen, start_y, start_x);
    for (int i = 0; i < length; i++) {
        switch (line[i]) {
            case '.':
                getyx(screen, y, x);
                wmove(screen, y, x + 1);
                break;
            case '#':
                if (mode == 1) {
                    waddch(screen, ACS_CKBOARD);
                } else {
                    waddch(screen, ' ');
                }
                break;
            case '\n':
                getyx(screen, y, x);
                wmove(screen, y + 1, start_x);
                break;
            case '\0':
                break;
            default:
                fprintf(stderr, "Unexpected character found: '%i'", line[i]);
                break;
        }
        wrefresh(screen);
    }
}

void startScreenExplodeStage(WINDOW *screen, int x, int y, char *string_from, char *string_to) {
    if (strcmp(string_from, "") != 1) {
        drawFromString(screen, x, y, string_from, 0);
    }
    if (strcmp(string_to, "") != 1) {
        drawFromString(screen, x, y, string_to, 1);
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

void *genHostileMissile(void *missiles) {
    struct missile *hostile_missiles = missiles;
    for (int i = 0; i < 10; i++) {
        usleep(1000000);
        hostile_missiles[i] = (struct missile) {
            .x = (rand() % (FRAME_WIDTH - 1)),
            .y = 0,
            .vel_x = (rand() % 3 - 1),
            .vel_y = 1,
        };
        if (hostile_missiles[i].x < FRAME_WIDTH / 2) {
            hostile_missiles[i].vel_x = (rand() % 2);
        } else {
            hostile_missiles[i].vel_x = (rand() % 2 - 1);
        }
        //mvaddch(hostile_missiles[i].y, hostile_missiles[i].x, '.');
    }
    return NULL;
}

void updateHostileMissile(WINDOW *screen, struct missile *missiles) {
    usleep(400000);
    for (int i = 0; i < 10; i++) {
        switch(missiles[i].vel_x) {
            case 1:
                mvwaddch(screen, missiles[i].y, missiles[i].x, '\\');
                break;
            case 0:
                mvwaddch(screen, missiles[i].y, missiles[i].x, '|');
                break;
            case -1:
                mvwaddch(screen, missiles[i].y, missiles[i].x, '/');
                break;
        }
        missiles[i].x += missiles[i].vel_x;
        missiles[i].y += missiles[i].vel_y;
        mvwaddch(screen, missiles[i].y, missiles[i].x, 'O');
    }
}

int main() {
    srand(time(0));
    //srand(6837);
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

    init_pair(14, COLOR_WHITE, COLOR_YELLOW);

    WINDOW *start_screen = newwin(FRAME_HEIGHT, FRAME_WIDTH, 0, 0);
    wattron(start_screen, A_BOLD);
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

    FILE *explosion_small_stage_1 = fopen("graphics/explosion-small-stage-1", "r");
    fseek (explosion_small_stage_1, 0, SEEK_END);
    int stage_1_length = ftell(explosion_small_stage_1);
    fseek (explosion_small_stage_1, 0, SEEK_SET);
    char *stage_1 = malloc(stage_1_length + 1);
    fread(stage_1, 1, stage_1_length, explosion_small_stage_1);
    stage_1[stage_1_length] = '\0';
    fclose(explosion_small_stage_1);

    FILE *explosion_small_stage_2 = fopen("graphics/explosion-small-stage-2", "r");
    fseek (explosion_small_stage_2, 0, SEEK_END);
    int stage_2_length = ftell(explosion_small_stage_2);
    fseek (explosion_small_stage_2, 0, SEEK_SET);
    char *stage_2 = malloc(stage_2_length + 1);
    fread(stage_2, 2, stage_2_length, explosion_small_stage_2);
    stage_2[stage_2_length] = '\0';
    fclose(explosion_small_stage_2);

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

    int cities_x_pos[6] = {15, 30, 45, 70, 85, 100};

    WINDOW *prep_screen = newwin(FRAME_HEIGHT, FRAME_WIDTH, 0, 0);
    wattron(prep_screen, A_BOLD);
    noecho();
    wattron(prep_screen, COLOR_PAIR(4));
    drawFromFile(prep_screen, 0, FRAME_HEIGHT - 6, "graphics/ground", 1);
    for (int i = 0; i < 6; i++) {
        wattron(prep_screen, COLOR_PAIR(3));
        drawFromFile(prep_screen, cities_x_pos[i], FRAME_HEIGHT - 4, "graphics/city-layer-1", 1);
        wattron(prep_screen, COLOR_PAIR(5));
        drawFromFile(prep_screen, cities_x_pos[i], FRAME_HEIGHT - 4, "graphics/city-layer-2", 1);
    }
    wattron(prep_screen, COLOR_PAIR(5));
    //mvwprintw(prep_screen, FRAME_HEIGHT - 6, FRAME_WIDTH / 4 - strlen("DEFEND") / 2, "DEFEND");
    drawFromFile(prep_screen, 18, FRAME_HEIGHT - 15, "graphics/defend-text", 1);
    drawFromFile(prep_screen, 72, FRAME_HEIGHT - 15, "graphics/cities-text", 1);
    wattron(prep_screen, COLOR_PAIR(2));
    for (int i = 0; i < 6; i++) {
        //drawFromFileExact(prep_screen, cities_x_pos[i] + 2, FRAME_HEIGHT - 7, "graphics/arrow-down-small", 1);
        mvwaddch(prep_screen, FRAME_HEIGHT - 7, cities_x_pos[i] + 3, 'V');
    }
    wattron(prep_screen, COLOR_PAIR(14));
    mvwprintw(prep_screen, FRAME_HEIGHT - 1, FRAME_WIDTH / 2 - strlen("Gabriel (Student ID: 37526367) @ 2019") / 2 - 2, "Gabriel (Student ID: 37526367) @ 2019");
    wgetch(prep_screen);
    werase(prep_screen);

    WINDOW *main_screen = newwin(FRAME_HEIGHT, FRAME_WIDTH, 0, 0);
    wattron(main_screen, A_BOLD);
    keypad(main_screen, TRUE);
    noecho();
    nodelay(main_screen, TRUE);
    wmove(main_screen, 0, 0);

    wattron(main_screen, COLOR_PAIR(4));
    drawFromFile(main_screen, 0, FRAME_HEIGHT - 6, "graphics/ground", 1);

    for (int i = 0; i < 6; i++) {
        wattron(main_screen, COLOR_PAIR(3));
        drawFromFile(main_screen, cities_x_pos[i], FRAME_HEIGHT - 4, "graphics/city-layer-1", 1);
        wattron(main_screen, COLOR_PAIR(5));
        drawFromFile(main_screen, cities_x_pos[i], FRAME_HEIGHT - 4, "graphics/city-layer-2", 1);
    }

    int input;
    int cur_x = 30;
    int cur_y = 10;

    struct missile hostile_missiles[10];

    wattron(main_screen, COLOR_PAIR(8));
    mvwaddch(main_screen, cur_y, cur_x, ACS_PLUS);

    pthread_t missile_gen_thread;
    pthread_create(&missile_gen_thread, NULL, genHostileMissile, hostile_missiles);

    while (1) {
        updateHostileMissile(main_screen, hostile_missiles);
        input = wgetch(main_screen);
        switch(input) {
            case KEY_LEFT:
                if (cur_x > 0) {
                    wattron(main_screen, COLOR_PAIR(8));
                    mvwaddch(main_screen, cur_y, cur_x, ' ');
                    cur_x -= 1;
                    mvwaddch(main_screen, cur_y, cur_x, ACS_PLUS);
                }
                break;
            case KEY_RIGHT:
                if (cur_x < FRAME_WIDTH - 1) {
                    wattron(main_screen, COLOR_PAIR(8));
                    mvwaddch(main_screen, cur_y, cur_x, ' ');
                    cur_x += 1;
                    mvwaddch(main_screen, cur_y, cur_x, ACS_PLUS);
                }
                break;
            case KEY_UP:
                if (cur_y > 0) {
                    wattron(main_screen, COLOR_PAIR(8));
                    mvwaddch(main_screen, cur_y, cur_x, ' ');
                    cur_y -= 1;
                    mvwaddch(main_screen, cur_y, cur_x, ACS_PLUS);
                }
                break;
            case KEY_DOWN:
                if (cur_y < FRAME_HEIGHT - 7) {
                    wattron(main_screen, COLOR_PAIR(8));
                    mvwaddch(main_screen, cur_y, cur_x, ' ');
                    cur_y += 1;
                    mvwaddch(main_screen, cur_y, cur_x, ACS_PLUS);
                }
                break;
            case 'q':
                endwin();
                break;
        }
    }

    //wgetch(main_screen);

    endwin();
}
