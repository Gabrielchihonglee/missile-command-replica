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
    int city_x, city_y;
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

static WINDOW *update_missile_thread_screen;
static int update_missile_thread_live;
static struct missile *update_missile_thread_hostile_missiles;
static struct missile *update_missile_thread_player_missiles;

char *STAGE_1, *STAGE_2;

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

    usleep(10000);

    pthread_mutex_lock(&lock);
    //mvwprintw(args->screen, args->city_y, city_x, "       ");
    //mvwprintw(args->screen, args->city_y + 1, city_x, "       ");
    drawFromString(args->screen, args->x, args->y, STAGE_2, DRAW);
    pthread_mutex_unlock(&lock);

    usleep(1000000);

    pthread_mutex_lock(&lock);
    drawFromString(args->screen, args->x, args->y, STAGE_2, ERASE);
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
                .city_x = cities_x_pos[i],
                .city_y = FRAME_HEIGHT - 4,
                .x = round(x),
                .y = round(y)
            };
            pthread_t missile_explosion_thread;
            pthread_create(&missile_explosion_thread, NULL, updateMissileExplosion, missile_explosion_args);
        }
    }
}

void *updateHostileMissiles(void *arguments) {
    int counter = 0;
    while (update_missile_thread_live) {
        usleep(20000);
        if (counter < 4) {
            counter++;
        } else {
            counter = 0;
        }
        if (!counter) {
            for (int i = 0; i < 10; i++) {
                if (update_missile_thread_hostile_missiles[i].live) {
                    if (abs(update_missile_thread_hostile_missiles[i].x - update_missile_thread_hostile_missiles[i].tar_x) < 0.00001 && abs(update_missile_thread_hostile_missiles[i].y - update_missile_thread_hostile_missiles[i].tar_y) < 0.00001) {
                        checkHitPlayer(update_missile_thread_screen, update_missile_thread_hostile_missiles[i].x, update_missile_thread_hostile_missiles[i].y);
                        killMissile(update_missile_thread_screen, &update_missile_thread_hostile_missiles[i]);
                    }

                    update_missile_thread_hostile_missiles[i].old_x = update_missile_thread_hostile_missiles[i].x;
                    update_missile_thread_hostile_missiles[i].old_y = update_missile_thread_hostile_missiles[i].y;
                    update_missile_thread_hostile_missiles[i].x += update_missile_thread_hostile_missiles[i].vel_x;
                    update_missile_thread_hostile_missiles[i].y += update_missile_thread_hostile_missiles[i].vel_y;

                    pthread_mutex_lock(&lock);
                    wattron(update_missile_thread_screen, COLOR_PAIR(2));
                    if (round(update_missile_thread_hostile_missiles[i].vel_x) > 0) {
                        mvwaddch(update_missile_thread_screen, round(update_missile_thread_hostile_missiles[i].old_y), round(update_missile_thread_hostile_missiles[i].old_x), '\\');
                    }
                    else if (round(update_missile_thread_hostile_missiles[i].vel_x) < 0) {
                        mvwaddch(update_missile_thread_screen, round(update_missile_thread_hostile_missiles[i].old_y), round(update_missile_thread_hostile_missiles[i].old_x), '/');
                    } else {
                        mvwaddch(update_missile_thread_screen, round(update_missile_thread_hostile_missiles[i].old_y), round(update_missile_thread_hostile_missiles[i].old_x), '|');
                    }
                    wattron(update_missile_thread_screen, COLOR_PAIR(8));
                    mvwaddch(update_missile_thread_screen, round(update_missile_thread_hostile_missiles[i].y), round(update_missile_thread_hostile_missiles[i].x), '.');
                    pthread_mutex_unlock(&lock);
                }
            }
        }
        for (int i = 0; i < 30; i++) {
            if (update_missile_thread_player_missiles[i].live) {
                if (abs(update_missile_thread_player_missiles[i].x - update_missile_thread_player_missiles[i].tar_x) < 0.00001 && abs(update_missile_thread_player_missiles[i].y - update_missile_thread_player_missiles[i].tar_y) < 0.00001) {
                    killMissile(update_missile_thread_screen, &update_missile_thread_player_missiles[i]);
                }

                update_missile_thread_player_missiles[i].old_x = update_missile_thread_player_missiles[i].x;
                update_missile_thread_player_missiles[i].old_y = update_missile_thread_player_missiles[i].y;
                update_missile_thread_player_missiles[i].x += update_missile_thread_player_missiles[i].vel_x;
                update_missile_thread_player_missiles[i].y += update_missile_thread_player_missiles[i].vel_y;

                pthread_mutex_lock(&lock);
                wattron(update_missile_thread_screen, COLOR_PAIR(5));
                if (round(update_missile_thread_player_missiles[i].vel_x) > 0) {
                    mvwaddch(update_missile_thread_screen, round(update_missile_thread_player_missiles[i].old_y), round(update_missile_thread_player_missiles[i].old_x), '/');
                }
                else if (round(update_missile_thread_player_missiles[i].vel_x) < 0) {
                    mvwaddch(update_missile_thread_screen, round(update_missile_thread_player_missiles[i].old_y), round(update_missile_thread_player_missiles[i].old_x), '\\');
                } else {
                    mvwaddch(update_missile_thread_screen, round(update_missile_thread_player_missiles[i].old_y), round(update_missile_thread_player_missiles[i].old_x), '|');
                }
                wattron(update_missile_thread_screen, COLOR_PAIR(8));
                mvwaddch(update_missile_thread_screen, round(update_missile_thread_player_missiles[i].y), round(update_missile_thread_player_missiles[i].x), '.');
                pthread_mutex_unlock(&lock);
            }
        }
    }
    return NULL;
}


void game() {
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

    int cities_x_pos[6] = {15, 30, 45, 70, 85, 100};

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
    for (int i = 0; i < 10; i++) {
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

    update_missile_thread_screen = main_screen;
    update_missile_thread_live = 1;
    update_missile_thread_hostile_missiles = hostile_missiles;
    update_missile_thread_player_missiles = player_missiles;
    pthread_t hostile_missiles_thread;
    pthread_create(&hostile_missiles_thread, NULL, updateHostileMissiles, NULL);

    int main_loop_run = 1;

    while (main_loop_run) {
        usleep(10000);
        pthread_mutex_lock(&lock);
        wrefresh(main_screen);
        pthread_mutex_unlock(&lock);
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
                update_missile_thread_live = 0;
                main_loop_run = 0;
                endwin();
                break;
        }
    }
}
