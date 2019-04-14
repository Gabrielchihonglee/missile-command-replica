#include "game.h"
#include "functions.h"

#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

struct missile {
    enum missileType type;
    int start_x, start_y;
    float x, y;
    float old_x, old_y;
    float vel_x, vel_y;
    int tar_x, tar_y;
    int live;
};

struct missileExplosionThreadArg {
    WINDOW *screen;
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
} cities[6];

struct base {
    int x, y;
    int missile_count;
} bases[3];

static WINDOW *game_screen;
static int game_live = 0;
static struct missile hostile_missiles[10] = {0};
static struct missile player_missiles[45] = {0};

char *STAGE_1, *STAGE_2;

void shootPlayerMissile(void *player_missiless, int tar_x, int tar_y, int base) {
    if (!bases[base - 1].missile_count) {
        updateMissileCount();
        return;
    }
    pthread_mutex_lock(&lock);
    wattron(game_screen, COLOR_PAIR(8));
    mvwaddch(game_screen, tar_y, tar_x, 'x');
    pthread_mutex_unlock(&lock);
    struct missile *player_missiles = player_missiless;
    float dist;
    struct missile player_missile = {
        .type = PLAYER,
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
    bases[base - 1].missile_count--;
    updateMissileCount();
}

void updateMissileCount() {
    char missile_count[3];
    for (int i = 0; i < 3; i++) {
        if (!bases[i].missile_count) {
            pthread_mutex_lock(&lock);
            wattron(game_screen, COLOR_PAIR(84));
            mvwprintw(game_screen, bases[i].y + 4, bases[i].x + 4, "OUT");
            pthread_mutex_unlock(&lock);
        } else {
            sprintf(missile_count, "%i", bases[i].missile_count);
            pthread_mutex_lock(&lock);
            wattron(game_screen, COLOR_PAIR(84));
            mvwprintw(game_screen, bases[i].y + 4, bases[i].x + 4, "   ");
            mvwprintw(game_screen, bases[i].y + 4, bases[i].x + 4, missile_count);
            pthread_mutex_unlock(&lock);
        }
    }
}

void killMissile(void *missile_input) {
    struct missile *missile = missile_input;
    pthread_mutex_lock(&lock);
    missile->live = 0;
    wattron(game_screen, COLOR_PAIR(1));
    mvwaddch(game_screen, missile->tar_y, missile->tar_x, ' ');
    while (1) {
        mvwaddch(game_screen, round(missile->y), round(missile->x), ' ');
        if (fabsf(missile->x - missile->start_x) < 0.1 && fabsf(missile->y - missile->start_y) < 0.1) {
            break;
        }
        missile->x -= missile->vel_x;
        missile->y -= missile->vel_y;
    }
    pthread_mutex_unlock(&lock);
}

void *updateMissileExplosion(void *arguments) {
    struct missileExplosionThreadArg *args = arguments;
    usleep(10000);
    pthread_mutex_lock(&lock);
    drawFromString(args->screen, args->x - 4, args->y - 2, LARGE_STAGE_1, DRAW);
    pthread_mutex_unlock(&lock);
    usleep(100000);
    pthread_mutex_lock(&lock);
    drawFromString(args->screen, args->x - 4, args->y - 2, LARGE_STAGE_2, DRAW);
    pthread_mutex_unlock(&lock);
    usleep(100000);
    pthread_mutex_lock(&lock);
    drawFromString(args->screen, args->x - 4, args->y - 2, LARGE_STAGE_2, ERASE);
    pthread_mutex_unlock(&lock);

    free(arguments);
    return NULL;
}

void checkHitPlayer(float x, float y) {
    for (int i = 0; i < 6; i++) {
        if ((x >= cities_x_pos[i]) && (x <= (cities_x_pos[i] + 6))) {
            pthread_mutex_lock(&lock);
            wattron(game_screen, COLOR_PAIR(3));
            mvwprintw(game_screen, FRAME_HEIGHT - 4, cities_x_pos[i], "       ");
            mvwprintw(game_screen, FRAME_HEIGHT - 3, cities_x_pos[i], "       ");
            pthread_mutex_unlock(&lock);
            cities[i].survive = -1;
            struct missileExplosionThreadArg *missile_explosion_args = malloc(sizeof(*missile_explosion_args));
            *missile_explosion_args = (struct missileExplosionThreadArg) {
                .screen = game_screen,
                .x = round(x),
                .y = round(y)
            };
            pthread_t missile_explosion_thread;
            pthread_create(&missile_explosion_thread, NULL, updateMissileExplosion, missile_explosion_args);
        }
    }
    for (int i = 0; i < 3; i++) {
        if ((x >= bases[i].x) && (x <= (bases[i].x + 7))) {
            bases[i].missile_count = 0;
            pthread_mutex_lock(&lock);
            drawFromFile(game_screen, bases[i].x, FRAME_HEIGHT - 6, "graphics/base", ERASE);
            pthread_mutex_unlock(&lock);
            updateMissileCount();
            struct missileExplosionThreadArg *missile_explosion_args = malloc(sizeof(*missile_explosion_args));
            *missile_explosion_args = (struct missileExplosionThreadArg) {
                .screen = game_screen,
                .x = round(x),
                .y = round(y)
            };
            pthread_t missile_explosion_thread;
            pthread_create(&missile_explosion_thread, NULL, updateMissileExplosion, missile_explosion_args);
        }
    }
}

void checkHitHostile(float x, float y) {
    for (int i = 0; i < 10; i++) {
        if (hostile_missiles[i].live && fabsf(x - hostile_missiles[i].x) < 5 && fabsf(y - hostile_missiles[i].y) < 5) {
            killMissile(&hostile_missiles[i]);
            struct missileExplosionThreadArg *missile_explosion_args = malloc(sizeof(*missile_explosion_args));
            *missile_explosion_args = (struct missileExplosionThreadArg) {
                .screen = game_screen,
                .x = round(x),
                .y = round(y)
            };
            pthread_t missile_explosion_thread;
            pthread_create(&missile_explosion_thread, NULL, updateMissileExplosion, missile_explosion_args);
            score += 25;
        }
    }
}

void *genHostileMissiles(void *arguments) {
    int rand_target_type;
    int rand_target_x, rand_target_y;
    float dist;
    for (int i = 0; i < 10; i++) {
        usleep(1000000);
        rand_target_type = rand() % 2 - 1;
        if (rand_target_type) {
            rand_target_x = cities_x_pos[rand() % 6] + rand() % 6;
            rand_target_y = 36;
        } else {
            rand_target_x = bases_x_pos[rand() % 3] + rand() % 6 + 2;
            rand_target_y = 33;
        }
        hostile_missiles[i] = (struct missile) {
            .live = 1,
            .type = HOSTILE_NORMAL,
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
    return NULL;
}

void *updateMissiles(void *arguments) {
    int counter = 0;
    while (game_live) {
        usleep(100000);
        if (counter < 4) {
            counter++;
        } else {
            counter = 0;
        }
        if (!counter) {
            for (int i = 0; i < 10; i++) {
                if (hostile_missiles[i].live) {
                    if (fabsf(hostile_missiles[i].x - hostile_missiles[i].tar_x) < 1 && fabsf(hostile_missiles[i].y - hostile_missiles[i].tar_y) < 1) {
                        checkHitPlayer(hostile_missiles[i].x, hostile_missiles[i].y);
                        killMissile(&hostile_missiles[i]);
                    } else {
                        hostile_missiles[i].old_x = hostile_missiles[i].x;
                        hostile_missiles[i].old_y = hostile_missiles[i].y;
                        hostile_missiles[i].x += hostile_missiles[i].vel_x;
                        hostile_missiles[i].y += hostile_missiles[i].vel_y;
                        pthread_mutex_lock(&lock);
                        wattron(game_screen, COLOR_PAIR(2));
                        if (round(hostile_missiles[i].vel_x) > 0) {
                            mvwaddch(game_screen, round(hostile_missiles[i].old_y), round(hostile_missiles[i].old_x), '\\');
                        }
                        else if (round(hostile_missiles[i].vel_x) < 0) {
                            mvwaddch(game_screen, round(hostile_missiles[i].old_y), round(hostile_missiles[i].old_x), '/');
                        } else {
                            mvwaddch(game_screen, round(hostile_missiles[i].old_y), round(hostile_missiles[i].old_x), '|');
                        }
                        wattron(game_screen, COLOR_PAIR(8));
                        mvwaddch(game_screen, round(hostile_missiles[i].y), round(hostile_missiles[i].x), '.');
                        pthread_mutex_unlock(&lock);
                    }
                }
            }
        }
        for (int i = 0; i < 30; i++) {
            if (player_missiles[i].live) {
                if (fabsf(player_missiles[i].x - player_missiles[i].tar_x) < 1 && fabsf(player_missiles[i].y - player_missiles[i].tar_y) < 1) {
                    checkHitHostile(player_missiles[i].x, player_missiles[i].y);
                    killMissile(&player_missiles[i]);
                }

                player_missiles[i].old_x = player_missiles[i].x;
                player_missiles[i].old_y = player_missiles[i].y;
                player_missiles[i].x += player_missiles[i].vel_x;
                player_missiles[i].y += player_missiles[i].vel_y;

                pthread_mutex_lock(&lock);
                wattron(game_screen, COLOR_PAIR(5));
                if (round(player_missiles[i].vel_x) > 0) {
                    mvwaddch(game_screen, round(player_missiles[i].old_y), round(player_missiles[i].old_x), '/');
                }
                else if (round(player_missiles[i].vel_x) < 0) {
                    mvwaddch(game_screen, round(player_missiles[i].old_y), round(player_missiles[i].old_x), '\\');
                } else {
                    mvwaddch(game_screen, round(player_missiles[i].old_y), round(player_missiles[i].old_x), '|');
                }
                wattron(game_screen, COLOR_PAIR(8));
                mvwaddch(game_screen, round(player_missiles[i].y), round(player_missiles[i].x), '.');
                pthread_mutex_unlock(&lock);
            }
        }
    }
    return NULL;
}

void moveCursor(int *cur_x, int *cur_y, int new_x, int new_y) {
    pthread_mutex_lock(&lock);
    wattron(game_screen, COLOR_PAIR(5));
    if ((mvwinch(game_screen, *cur_y, *cur_x) & A_CHARTEXT) != ('x' & A_CHARTEXT)) {
        mvwaddch(game_screen, *cur_y, *cur_x, ' ');
    }
    *cur_x = new_x;
    *cur_y = new_y;
    mvwaddch(game_screen, *cur_y, *cur_x, '+');
    pthread_mutex_unlock(&lock);
}

void *inputListener(void *arguments) {
    mousemask(ALL_MOUSE_EVENTS, NULL);
    int input;
    int cur_x = FRAME_WIDTH / 2;
    int cur_y = (FRAME_HEIGHT - 7) / 2;
    wattron(game_screen, COLOR_PAIR(5));
    mvwaddch(game_screen, cur_y, cur_x, '+');
    MEVENT event;
    while (game_live) {
        input = wgetch(game_screen);
        switch (input) {
            case KEY_MOUSE:
                if (getmouse(&event) == OK) {
                    if (event.x < FRAME_WIDTH && event.y < (FRAME_HEIGHT - 7)) {
                        moveCursor(&cur_x, &cur_y, event.x, event.y);
                    }
                  }
                break;
            case KEY_LEFT:
                if (cur_x > 0) {
                    moveCursor(&cur_x, &cur_y, cur_x - 1, cur_y);
                }
                break;
            case KEY_RIGHT:
                if (cur_x < FRAME_WIDTH - 1) {
                    moveCursor(&cur_x, &cur_y, cur_x + 1, cur_y);
                }
                break;
            case KEY_UP:
                if (cur_y > 0) {
                    moveCursor(&cur_x, &cur_y, cur_x, cur_y - 1);
                }
                break;
            case KEY_DOWN:
                if (cur_y < FRAME_HEIGHT - 7) {
                    moveCursor(&cur_x, &cur_y, cur_x, cur_y + 1);
                }
                break;
            case '1':
                shootPlayerMissile(player_missiles, cur_x, cur_y, 1);
                break;
            case '2':
                shootPlayerMissile(player_missiles, cur_x, cur_y, 2);
                break;
            case '3':
                shootPlayerMissile(player_missiles, cur_x, cur_y, 3);
                break;
            case 'q':
                game_live = 0;
                endwin();
                break;
        }
    }
    return NULL;
}

void game() {
    WINDOW *main_screen = newwin(FRAME_HEIGHT, FRAME_WIDTH, 0, 0);
    game_live = 1;
    wattron(main_screen, A_BOLD);
    keypad(main_screen, TRUE);
    noecho();
    wmove(main_screen, 0, 0);
    werase(main_screen);
    erase();

    drawScreenSettings(main_screen, 0);
    wrefresh(main_screen);

    game_screen = main_screen;

    for (int i = 0; i < 3; i++) {
        bases[i] = (struct base) {
            .x = bases_x_pos[i],
            .y = FRAME_HEIGHT - 6,
            .missile_count = 10
        };
    }
    updateMissileCount();

    pthread_t update_missiles_thread;
    pthread_create(&update_missiles_thread, NULL, updateMissiles, NULL);

    pthread_t gen_hostile_missiles_thread;
    pthread_create(&gen_hostile_missiles_thread, NULL, genHostileMissiles, NULL);

    pthread_t input_listener_thread;
    pthread_create(&input_listener_thread, NULL, inputListener, NULL);

    while (game_live) {
        usleep(10000);
        pthread_mutex_lock(&lock);
        wrefresh(main_screen);
        refreshHighScore(main_screen);
        pthread_mutex_unlock(&lock);
    }
}
