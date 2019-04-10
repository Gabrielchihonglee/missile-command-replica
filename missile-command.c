#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#define FRAME_WIDTH 124
#define FRAME_HEIGHT 40

#define START_PADDING_HORIZONTAL 30
#define START_PADDING_VERTICAL 12

struct flashThreadArg {
    WINDOW *screen;
    int live;
    int x, y;
    char *text;
    int duration;
    int color_pair;
};

struct carouselThreadArg {
    WINDOW *screen;
    int live;
    int start_x, end_x, y;
    char *text;
    int color_pair;
};

struct missile {
    int type; // 0: player; 1: hostile normal; 2: hostile crazy
    int start_x, start_y;
    float x, y;
    float old_x, old_y;
    float vel_x, vel_y;
    int tar_x, tar_y;
    int live;
};

struct missileExplosionThreadArg {
    WINDOW *screen;
    int clean_x, clean_y;
    int x, y;
};

struct hostileMissilesThreadArg {
    WINDOW *screen;
    int live;
    struct missile *hostile_missiles;
    struct missile *player_missiles;
};

struct city {
    int x, y;
    int survive;
};

struct base {
    int x, y;
    int num_of_missiles;
};

static pthread_mutex_t lock;

static int start_explosion_pos[80][2];
static char *STAGE_1, *STAGE_2;

static enum drawMode {ERASE, DRAW};

static struct carouselThreadArg *prep_screen_carousel_args;
static struct flashThreadArg *prep_screen_arrow_args;

static WINDOW *flash_thread_screen;
static int flash_thread_live;
static int flash_thread_x, flash_thread_y;
static char *flash_thread_text;
static int flash_thread_duration;
static int flash_thread_color_pair;

static WINDOW *carousel_thread_screen;
static int carousel_thread_live;
static int carousel_thread_start_x, carousel_thread_end_x, carousel_thread_y;
static char *carousel_thread_text;
static int carousel_thread_color_pair;

void drawFromFile(WINDOW *screen, int start_x, int start_y, char file[], enum drawMode mode) { // mode 1: draw 0: erase/draw with backgound
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
    }
}

void drawFromString(WINDOW *screen, int start_x, int start_y, char *line, enum drawMode mode) {
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
            default:
                fprintf(stderr, "Unexpected character found: '%i'", line[i]);
                break;
        }
        wrefresh(screen);
    }
}

void updateSmallExplosionStage(WINDOW *screen, int from_missile, int to_missile, int color) {
    pthread_mutex_lock(&lock);
    wattron(screen, COLOR_PAIR(color));
    for (int i = from_missile; i < to_missile; i++) {
        drawFromString(screen, start_explosion_pos[i][0], start_explosion_pos[i][1], STAGE_1, DRAW); // draw stage 1
    }
    pthread_mutex_unlock(&lock);
    usleep(100000);
    pthread_mutex_lock(&lock);
    wattron(screen, COLOR_PAIR(color));
    for (int i = from_missile; i < to_missile; i++) {
        drawFromString(screen, start_explosion_pos[i][0], start_explosion_pos[i][1], STAGE_2, DRAW); // draw stage 2
    }
    pthread_mutex_unlock(&lock);
    usleep(100000);
    pthread_mutex_lock(&lock);
    wattron(screen, COLOR_PAIR(color));
    for (int i = from_missile; i < to_missile; i++) {
        drawFromString(screen, start_explosion_pos[i][0], start_explosion_pos[i][1], STAGE_2, ERASE); // erase stage 2
    }
    pthread_mutex_unlock(&lock);
    usleep(1000);
}

void startScreenTextColor(WINDOW *screen, int color) {
    wattron(screen, COLOR_PAIR(color));
    pthread_mutex_lock(&lock);
    for (int i = START_PADDING_VERTICAL; i < (16 + START_PADDING_VERTICAL); i++) {
        for (int j = START_PADDING_HORIZONTAL; j < (63 + START_PADDING_HORIZONTAL); j++) {
            if ((mvwinch(screen, i, j) & A_CHARTEXT) == (ACS_CKBOARD & A_CHARTEXT)) {
                mvwaddch(screen, i, j, ACS_CKBOARD);
                wrefresh(screen);
            }
        }
    }
    pthread_mutex_unlock(&lock);
}

void refreshHighScore(WINDOW *screen, int cur_score, int high_score) {
    char cur_score_text[10];
    char high_score_text[10];
    sprintf(cur_score_text, "%i", cur_score);
    sprintf(high_score_text, "%i", high_score);
    wattron(screen, COLOR_PAIR(2));
    mvwprintw(screen, 0, FRAME_WIDTH / 2, cur_score_text);
    mvwprintw(screen, 0, FRAME_WIDTH / 2 - 15, cur_score_text);
}

void shootPlayerMissile(void *player_missiless, int tar_x, int tar_y, int base) {
    struct missile *player_missiles = player_missiless;
    float dist;
    struct missile player_missile = {
        .type = 0,
        .start_x =  base * 55 - 50,
        .x = base * 55 - 50,
        .start_y = FRAME_HEIGHT - 7,
        .y = FRAME_HEIGHT - 7,
        .vel_x = 0,
        .vel_y = 0,
        .tar_x = tar_x,
        .tar_y = tar_y,
        .live = 1
    };
    dist = sqrtf(pow(player_missile.x - player_missile.tar_x, 2) + pow(player_missile.y - player_missile.tar_y, 2));
    player_missile.vel_x = (player_missile.tar_x - player_missile.x) / dist * 2;
    player_missile.vel_y = (player_missile.tar_y - player_missile.y) / dist * 2;
    for (int i = 0; i < 30; i++) {
        if (!player_missiles[i].live) {
            player_missiles[i] = player_missile;
            break;
        }
    }
}

void killMissile(WINDOW *screen, void *missile_input) {
    struct missile *missile = missile_input;
    missile->live = 0;
    int remove_trace_complete = 0;
    pthread_mutex_lock(&lock);
    while (!remove_trace_complete) {
        if (abs(missile->x - missile->start_x) < 0.00001 && abs(missile->y - missile->start_y) < 0.00001) {
            remove_trace_complete = 1;
        }
        mvwaddch(screen, round(missile->y), round(missile->x), ' ');
        missile->x -= missile->vel_x;
        missile->y -= missile->vel_y;
    }
    pthread_mutex_unlock(&lock);
}

void *updateMissileExplosion(void *arguments) {
    struct missileExplosionThreadArg *args = arguments;
    WINDOW *screen = args->screen;
    int clean_x = args->clean_x;
    int clean_y = args->clean_y;
    int x = args->x;
    int y = args->y;
    pthread_mutex_lock(&lock);
    usleep(10000);
    mvwprintw(screen, clean_y, clean_x, "       ");
    mvwprintw(screen, clean_y + 1, clean_x, "       ");
    FILE *explosion_small_stage_2 = fopen("graphics/explosion-small-stage-2", "r");
    fseek (explosion_small_stage_2, 0, SEEK_END);
    int stage_2_length = ftell(explosion_small_stage_2);
    fseek (explosion_small_stage_2, 0, SEEK_SET);
    char *stage_2 = malloc(stage_2_length + 1);
    fread(stage_2, 2, stage_2_length, explosion_small_stage_2);
    stage_2[stage_2_length] = '\0';
    fclose(explosion_small_stage_2);
    drawFromString(screen, x, y, stage_2, DRAW);
    pthread_mutex_unlock(&lock);
    usleep(1000000);
    pthread_mutex_lock(&lock);
    drawFromString(screen, x, y, stage_2, ERASE);
    pthread_mutex_unlock(&lock);
    free(arguments);
    return NULL;
}

void checkHitPlayer(WINDOW *screen, float x, float y) {
    int cities_x_pos[6] = {15, 30, 45, 70, 85, 100};
    for (int i = 0; i < 6; i++) {
        if (abs(x - cities_x_pos[i] - 3) < 1) {
            struct missileExplosionThreadArg *missile_explosion_args = malloc(sizeof(*missile_explosion_args));
            *missile_explosion_args = (struct missileExplosionThreadArg) {
                .screen = screen,
                .clean_x = cities_x_pos[i],
                .clean_y = FRAME_HEIGHT - 4,
                .x = round(x),
                .y = round(y)
            };
            pthread_t missile_explosion_thread;
            pthread_create(&missile_explosion_thread, NULL, updateMissileExplosion, missile_explosion_args);
        }
    }
}

void *flashFromString(void *arguments) {
    while (flash_thread_live) {
        pthread_mutex_lock(&lock);
        wattron(flash_thread_screen, COLOR_PAIR(flash_thread_color_pair));
        mvwprintw(flash_thread_screen, flash_thread_y, flash_thread_x, flash_thread_text);
        wrefresh(flash_thread_screen);
        pthread_mutex_unlock(&lock);
        usleep(flash_thread_duration / 2);
        pthread_mutex_lock(&lock);
        wattron(flash_thread_screen, COLOR_PAIR(flash_thread_color_pair));
        for (int i = 0; i < strlen(flash_thread_text) - 1; i++) {
            mvwprintw(flash_thread_screen, flash_thread_y, flash_thread_x + i, " ");
            wrefresh(flash_thread_screen);
        }
        pthread_mutex_unlock(&lock);
        usleep(flash_thread_duration / 2);
    }
    return NULL;
}

void *carouselFromString(void *argument) {
    wrefresh(carousel_thread_screen);
    int carousel_thread_head_x = carousel_thread_start_x;
    while (carousel_thread_live) {
        pthread_mutex_lock(&lock);
        for (int i = 0; i < (FRAME_WIDTH - 1 - carousel_thread_head_x) && i < strlen(carousel_thread_text); i++) {
            if ((carousel_thread_head_x + i) < carousel_thread_end_x) {
                continue;
            }
            wattron(carousel_thread_screen, COLOR_PAIR(carousel_thread_color_pair));
            mvwaddch(carousel_thread_screen, carousel_thread_y, carousel_thread_head_x + i, carousel_thread_text[i]);
        }
        if ((carousel_thread_head_x + strlen(carousel_thread_text)) < (FRAME_WIDTH - 1)) {
            wattron(carousel_thread_screen, COLOR_PAIR(carousel_thread_color_pair));
            mvwaddch(carousel_thread_screen, carousel_thread_y, carousel_thread_head_x + strlen(carousel_thread_text), ' ');
        }
        pthread_mutex_unlock(&lock);
        wrefresh(carousel_thread_screen);
        usleep(160000);
        carousel_thread_head_x--;
        if (carousel_thread_head_x + strlen(carousel_thread_text) + 1 <= carousel_thread_end_x) {
            carousel_thread_head_x = carousel_thread_start_x;
        }
    }
    return NULL;
}

void *updateHostileMissiles(void *arguments) {
    struct hostileMissilesThreadArg *args = arguments;
    WINDOW *screen = args->screen;
    struct missile *hostile_missiles = args->hostile_missiles;
    struct missile *player_missiles = args->player_missiles;
    int counter = 0;
    while (args->live) {
        usleep(20000);
        if (counter < 4) {
            counter++;
        } else {
            counter = 0;
        }
        if (!counter) {
            for (int i = 0; i < 5; i++) {
                if (hostile_missiles[i].live) {
                    if (abs(hostile_missiles[i].x - hostile_missiles[i].tar_x) < 0.00001 && abs(hostile_missiles[i].y - hostile_missiles[i].tar_y) < 0.00001) {
                        checkHitPlayer(screen, hostile_missiles[i].x, hostile_missiles[i].y);
                        killMissile(screen, &hostile_missiles[i]);
                    }
                    hostile_missiles[i].old_x = hostile_missiles[i].x;
                    hostile_missiles[i].old_y = hostile_missiles[i].y;
                    hostile_missiles[i].x += hostile_missiles[i].vel_x;
                    hostile_missiles[i].y += hostile_missiles[i].vel_y;
                }
            }
        }
        for (int i = 0; i < 30; i++) {
            if (player_missiles[i].live) {
                if (abs(player_missiles[i].x - player_missiles[i].tar_x) < 0.00001 && abs(player_missiles[i].y - player_missiles[i].tar_y) < 0.00001) {
                    killMissile(screen, &player_missiles[i]);
                }
                player_missiles[i].old_x = player_missiles[i].x;
                player_missiles[i].old_y = player_missiles[i].y;
                player_missiles[i].x += player_missiles[i].vel_x;
                player_missiles[i].y += player_missiles[i].vel_y;
            }
        }
    }
    return NULL;
}

int main() {
    srand(time(0));
    initscr();
    raw();
    curs_set(0);

    pthread_mutex_init(&lock, NULL);

    start_color();
    init_pair(1, COLOR_BLACK, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    init_pair(5, COLOR_BLUE, COLOR_BLACK);
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(7, COLOR_CYAN, COLOR_BLACK);
    init_pair(8, COLOR_WHITE, COLOR_BLACK);

    init_pair(84, COLOR_WHITE, COLOR_YELLOW);

    WINDOW *start_screen = newwin(FRAME_HEIGHT, FRAME_WIDTH, 0, 0);
    wattron(start_screen, A_BOLD);
    wattron(start_screen, COLOR_PAIR(2));
    drawFromFile(start_screen, START_PADDING_HORIZONTAL, START_PADDING_VERTICAL, "graphics/missile-command-text", DRAW);
    wrefresh(start_screen);
    usleep(1000000);

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
    STAGE_1 = malloc(sizeof(char) * stage_1_length + 1);
    fseek (explosion_small_stage_1, 0, SEEK_SET);
    fread(STAGE_1, 1, stage_1_length, explosion_small_stage_1);
    STAGE_1[stage_1_length] = '\0';
    fclose(explosion_small_stage_1);

    FILE *explosion_small_stage_2 = fopen("graphics/explosion-small-stage-2", "r");
    fseek (explosion_small_stage_2, 0, SEEK_END);
    int stage_2_length = ftell(explosion_small_stage_2);
    STAGE_2 = malloc(sizeof(char) * stage_2_length + 1);
    fseek (explosion_small_stage_2, 0, SEEK_SET);
    fread(STAGE_2, 2, stage_2_length, explosion_small_stage_2);
    STAGE_2[stage_2_length] = '\0';
    fclose(explosion_small_stage_2);

    for (int i = 0; i < 2; i++) {
        updateSmallExplosionStage(start_screen, 0, 10, 4);
        startScreenTextColor(start_screen, 3);
        updateSmallExplosionStage(start_screen, 10, 20, 5);
        startScreenTextColor(start_screen, 4);
        updateSmallExplosionStage(start_screen, 20, 30, 6);
        startScreenTextColor(start_screen, 5);
        updateSmallExplosionStage(start_screen, 30, 40, 7);
        startScreenTextColor(start_screen, 6);
        updateSmallExplosionStage(start_screen, 40, 50, 8);
        startScreenTextColor(start_screen, 7);
        updateSmallExplosionStage(start_screen, 50, 60, 1);
        startScreenTextColor(start_screen, 8);
        updateSmallExplosionStage(start_screen, 60, 70, 2);
        startScreenTextColor(start_screen, 1);
        updateSmallExplosionStage(start_screen, 70, 80, 3);
        startScreenTextColor(start_screen, 2);
    }

    usleep(1000000);
    wattron(start_screen, COLOR_PAIR(84));
    mvwprintw(start_screen, FRAME_HEIGHT - 1, FRAME_WIDTH / 2 - strlen("PRESS ANY KEY TO CONTINUE") / 2, "PRESS ANY KEY TO CONTINUE");
    wgetch(start_screen);
    werase(start_screen);
    delwin(start_screen);

    WINDOW *prep_screen = newwin(FRAME_HEIGHT, FRAME_WIDTH, 0, 0);
    wattron(prep_screen, A_BOLD);
    noecho();
    wattron(prep_screen, COLOR_PAIR(84));
    drawFromFile(prep_screen, 0, FRAME_HEIGHT - 6, "graphics/ground", ERASE);
    int cities_x_pos[6] = {15, 30, 45, 70, 85, 100};
    for (int i = 0; i < 6; i++) {
        wattron(prep_screen, COLOR_PAIR(3));
        drawFromFile(prep_screen, cities_x_pos[i], FRAME_HEIGHT - 4, "graphics/city-layer-1", DRAW);
        wattron(prep_screen, COLOR_PAIR(5));
        drawFromFile(prep_screen, cities_x_pos[i], FRAME_HEIGHT - 4, "graphics/city-layer-2", DRAW);
    }
    wattron(prep_screen, COLOR_PAIR(5));
    drawFromFile(prep_screen, 18, FRAME_HEIGHT - 15, "graphics/defend-text", DRAW);
    drawFromFile(prep_screen, 72, FRAME_HEIGHT - 15, "graphics/cities-text", DRAW);

    int cur_score = 20;
    int high_score = 0;
    refreshHighScore(prep_screen, cur_score, high_score);

    wattron(prep_screen, COLOR_PAIR(2));
    char prep_screen_arrow_string[FRAME_WIDTH - 1];
    for (int i = 0; i < FRAME_WIDTH - 1; i++) {
        prep_screen_arrow_string[i] = ' ';
    }
    for (int i = 0; i < 6; i++) {
        prep_screen_arrow_string[cities_x_pos[i] + 3] = 'V';
    }
    prep_screen_arrow_string[FRAME_WIDTH - 1] = '\0';
    flash_thread_screen = prep_screen;
    flash_thread_live = 1;
    flash_thread_x = 0;
    flash_thread_y = FRAME_HEIGHT - 7;
    flash_thread_text = prep_screen_arrow_string;
    flash_thread_duration = 1200000;
    flash_thread_color_pair = 2;
    pthread_t prep_screen_arrow_thread;
    pthread_create(&prep_screen_arrow_thread, NULL, flashFromString, NULL);

    usleep(100000);

    carousel_thread_screen = prep_screen;
    carousel_thread_live = 1;
    carousel_thread_start_x = FRAME_WIDTH - 1;
    carousel_thread_end_x = 0;
    carousel_thread_y = FRAME_HEIGHT - 1;
    carousel_thread_text = "GABRIEL (LANC UNI ID: 37526367) @ 2019     INSERT COINS     1 COIN 1 PLAY";
    carousel_thread_color_pair = 84;
    pthread_t prep_screen_carousel_thread;
    pthread_create(&prep_screen_carousel_thread, NULL, carouselFromString, NULL);

    wgetch(prep_screen);
    flash_thread_live = 0;
    carousel_thread_live = 0;
    werase(prep_screen);
    delwin(prep_screen);

    WINDOW *main_screen = newwin(FRAME_HEIGHT, FRAME_WIDTH, 0, 0);
    wattron(main_screen, A_BOLD);
    keypad(main_screen, TRUE);
    noecho();
    nodelay(main_screen, TRUE);
    wmove(main_screen, 0, 0);
    werase(main_screen);
    erase();

    wattron(main_screen, COLOR_PAIR(84));
    drawFromFile(main_screen, 0, FRAME_HEIGHT - 6, "graphics/ground", ERASE);

    for (int i = 0; i < 6; i++) {
        wattron(main_screen, COLOR_PAIR(3));
        drawFromFile(main_screen, cities_x_pos[i], FRAME_HEIGHT - 4, "graphics/city-layer-1", DRAW);
        wattron(main_screen, COLOR_PAIR(5));
        drawFromFile(main_screen, cities_x_pos[i], FRAME_HEIGHT - 4, "graphics/city-layer-2", DRAW);
    }

    wrefresh(main_screen);

    int input;
    int cur_x = 30;
    int cur_y = 10;

    struct missile hostile_missiles[10] = {0};
    struct missile player_missiles[30] = {0};

    wattron(main_screen, COLOR_PAIR(8));
    mvwaddch(main_screen, cur_y, cur_x, ACS_PLUS);

    int base_x_pos[3] = {8, 60, 115};

    int rand_target_type;
    int rand_target_x, rand_target_y;
    float dist;
    for (int i = 0; i < 5; i++) {
        rand_target_type = rand() % 2 - 1;
        if (rand_target_type) {
            rand_target_x = cities_x_pos[rand() % 6] + 3;
            rand_target_y = 36;
        } else {
            rand_target_x = base_x_pos[rand() % 3];
            rand_target_y = 33;
        }
        hostile_missiles[i] = (struct missile) {
            .live = 1,
            .type = 1,
            .start_x = (rand() % (FRAME_WIDTH - 1)),
            .start_y = 0,
            .y = 0,
            .vel_x = 0,
            .vel_y = 0,
            .tar_x = rand_target_x,
            .tar_y = rand_target_y
        };
        hostile_missiles[i].x = hostile_missiles[i].start_x;
        dist = sqrtf(pow(hostile_missiles[i].x - rand_target_x, 2) + rand_target_y * rand_target_y);
        hostile_missiles[i].vel_x = (rand_target_x - hostile_missiles[i].x) / dist;
        hostile_missiles[i].vel_y = rand_target_y / dist;
    }

    struct hostileMissilesThreadArg *hostile_missiles_args = malloc(sizeof(*hostile_missiles_args));
    *hostile_missiles_args = (struct hostileMissilesThreadArg) {
        .screen = main_screen,
        .live = 1,
        .hostile_missiles = hostile_missiles,
        .player_missiles = player_missiles
    };
    pthread_t hostile_missiles_thread;
    pthread_create(&hostile_missiles_thread, NULL, updateHostileMissiles, hostile_missiles_args);

    while (1) {
        for (int i = 0; i < 5; i++) {
            if (hostile_missiles[i].live) {
                pthread_mutex_lock(&lock);
                wattron(main_screen, COLOR_PAIR(2));
                if (round(hostile_missiles[i].vel_x) > 0) {
                    mvwaddch(main_screen, round(hostile_missiles[i].old_y), round(hostile_missiles[i].old_x), '\\');
                }
                else if (round(hostile_missiles[i].vel_x) < 0) {
                    mvwaddch(main_screen, round(hostile_missiles[i].old_y), round(hostile_missiles[i].old_x), '/');
                } else {
                    mvwaddch(main_screen, round(hostile_missiles[i].old_y), round(hostile_missiles[i].old_x), '|');
                }
                wattron(main_screen, COLOR_PAIR(8));
                mvwaddch(main_screen, round(hostile_missiles[i].y), round(hostile_missiles[i].x), '.');
                pthread_mutex_unlock(&lock);
            }
        }
        for (int i = 0; i < 30; i++) {
            if (player_missiles[i].live) {
                pthread_mutex_lock(&lock);
                wattron(main_screen, COLOR_PAIR(5));
                if (round(player_missiles[i].vel_x) > 0) {
                    mvwaddch(main_screen, round(player_missiles[i].old_y), round(player_missiles[i].old_x), '/');
                }
                else if (round(player_missiles[i].vel_x) < 0) {
                    mvwaddch(main_screen, round(player_missiles[i].old_y), round(player_missiles[i].old_x), '\\');
                } else {
                    mvwaddch(main_screen, round(player_missiles[i].old_y), round(player_missiles[i].old_x), '|');
                }
                wattron(main_screen, COLOR_PAIR(8));
                mvwaddch(main_screen, round(player_missiles[i].y), round(player_missiles[i].x), '.');
                pthread_mutex_unlock(&lock);
            }
        }
        wrefresh(main_screen);
        input = wgetch(main_screen);
        switch(input) {
            case KEY_LEFT:
                pthread_mutex_lock(&lock);
                if (cur_x > 0) {
                    wattron(main_screen, COLOR_PAIR(8));
                    mvwaddch(main_screen, cur_y, cur_x, ' ');
                    cur_x -= 1;
                    mvwaddch(main_screen, cur_y, cur_x, ACS_PLUS);
                }
                pthread_mutex_unlock(&lock);
                break;
            case KEY_RIGHT:
                pthread_mutex_lock(&lock);
                if (cur_x < FRAME_WIDTH - 1) {
                    wattron(main_screen, COLOR_PAIR(8));
                    mvwaddch(main_screen, cur_y, cur_x, ' ');
                    cur_x += 1;
                    mvwaddch(main_screen, cur_y, cur_x, ACS_PLUS);
                }
                pthread_mutex_unlock(&lock);
                break;
            case KEY_UP:
                pthread_mutex_lock(&lock);
                if (cur_y > 0) {
                    wattron(main_screen, COLOR_PAIR(8));
                    mvwaddch(main_screen, cur_y, cur_x, ' ');
                    cur_y -= 1;
                    mvwaddch(main_screen, cur_y, cur_x, ACS_PLUS);
                }
                pthread_mutex_unlock(&lock);
                break;
            case KEY_DOWN:
                pthread_mutex_lock(&lock);
                if (cur_y < FRAME_HEIGHT - 7) {
                    wattron(main_screen, COLOR_PAIR(8));
                    mvwaddch(main_screen, cur_y, cur_x, ' ');
                    cur_y += 1;
                    mvwaddch(main_screen, cur_y, cur_x, ACS_PLUS);
                }
                pthread_mutex_unlock(&lock);
                break;
            case '1':
                shootPlayerMissile(&player_missiles, cur_x, cur_y, 1);
                break;
            case '2':
                shootPlayerMissile(player_missiles, cur_x, cur_y, 2);
                break;
            case '3':
                shootPlayerMissile(player_missiles, cur_x, cur_y, 3);
                break;
            case 'q':
                hostile_missiles_args->live = 0;
                endwin();
                return 0;
                break;
        }
    }

    endwin();
    pthread_mutex_destroy(&lock);
}
