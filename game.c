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
    int missile_count;
} bases[3];

static WINDOW *game_screen;
static int update_missile_thread_live;
static struct missile *update_missile_thread_hostile_missiles;
static struct missile *update_missile_thread_player_missiles;

char *STAGE_1, *STAGE_2;
int main_loop_run = 1;

void shootPlayerMissile(void *player_missiless, int tar_x, int tar_y, int base) {
    if (!bases[base - 1].missile_count) {
        updateMissileCount();
        return;
    }
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

void killMissile(WINDOW *screen, void *missile_input) {
    struct missile *missile = missile_input;
    pthread_mutex_lock(&lock);
    while (1) {
        mvwaddch(screen, round(missile->y), round(missile->x), ' ');
        if (fabsf(missile->x - missile->start_x) < 0.00001 && fabsf(missile->y - missile->start_y) < 0.00001) {
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

void checkHitPlayer(WINDOW *screen, float x, float y) {
    int cities_x_pos[6] = {15, 30, 45, 70, 85, 100};
    for (int i = 0; i < 6; i++) {
        //if (abs(x - cities_x_pos[i] - 3) < 1) {
        if ((x >= cities_x_pos[i]) && (x <= (cities_x_pos[i] + 6))) {
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
    for (int i = 0; i < 3; i++) {
        if ((x >= bases[i].x) && (x <= (bases[i].x + 7))) {
            struct missileExplosionThreadArg *missile_explosion_args = malloc(sizeof(*missile_explosion_args));
            *missile_explosion_args = (struct missileExplosionThreadArg) {
                .screen = screen,
                .city_x = bases[i].x,
                .city_y = FRAME_HEIGHT - 4,
                .x = round(x),
                .y = round(y)
            };
            pthread_t missile_explosion_thread;
            pthread_create(&missile_explosion_thread, NULL, updateMissileExplosion, missile_explosion_args);
        }
    }
}

void *genHostileMissiles(void *arguments) {
    int cities_x_pos[6] = {15, 30, 45, 70, 85, 100};
    int base_x_pos[3] = {6, 60, 117};

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
            rand_target_x = base_x_pos[rand() % 3];
            rand_target_y = 33;
        }
        update_missile_thread_hostile_missiles[i] = (struct missile) {
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
        update_missile_thread_hostile_missiles[i].x = update_missile_thread_hostile_missiles[i].start_x;
        dist = sqrtf(pow(update_missile_thread_hostile_missiles[i].x - rand_target_x, 2) + rand_target_y * rand_target_y);
        update_missile_thread_hostile_missiles[i].vel_x = (rand_target_x - update_missile_thread_hostile_missiles[i].x) / dist;
        update_missile_thread_hostile_missiles[i].vel_y = rand_target_y / dist;
    }
    return NULL;
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
                    if (fabsf(update_missile_thread_hostile_missiles[i].x - update_missile_thread_hostile_missiles[i].tar_x) < 1 && fabsf(update_missile_thread_hostile_missiles[i].y - update_missile_thread_hostile_missiles[i].tar_y) < 1) {
                        update_missile_thread_hostile_missiles[i].live = 0;
                        checkHitPlayer(game_screen, update_missile_thread_hostile_missiles[i].x, update_missile_thread_hostile_missiles[i].y);
                        killMissile(game_screen, &update_missile_thread_hostile_missiles[i]);
                    } else {
                        update_missile_thread_hostile_missiles[i].old_x = update_missile_thread_hostile_missiles[i].x;
                        update_missile_thread_hostile_missiles[i].old_y = update_missile_thread_hostile_missiles[i].y;
                        update_missile_thread_hostile_missiles[i].x += update_missile_thread_hostile_missiles[i].vel_x;
                        update_missile_thread_hostile_missiles[i].y += update_missile_thread_hostile_missiles[i].vel_y;
                        pthread_mutex_lock(&lock);
                        wattron(game_screen, COLOR_PAIR(2));
                        if (round(update_missile_thread_hostile_missiles[i].vel_x) > 0) {
                            mvwaddch(game_screen, round(update_missile_thread_hostile_missiles[i].old_y), round(update_missile_thread_hostile_missiles[i].old_x), '\\');
                        }
                        else if (round(update_missile_thread_hostile_missiles[i].vel_x) < 0) {
                            mvwaddch(game_screen, round(update_missile_thread_hostile_missiles[i].old_y), round(update_missile_thread_hostile_missiles[i].old_x), '/');
                        } else {
                            mvwaddch(game_screen, round(update_missile_thread_hostile_missiles[i].old_y), round(update_missile_thread_hostile_missiles[i].old_x), '|');
                        }
                        wattron(game_screen, COLOR_PAIR(8));
                        mvwaddch(game_screen, round(update_missile_thread_hostile_missiles[i].y), round(update_missile_thread_hostile_missiles[i].x), '.');
                        pthread_mutex_unlock(&lock);
                    }
                }
            }
        }
        for (int i = 0; i < 30; i++) {
            if (update_missile_thread_player_missiles[i].live) {
                if (fabsf(update_missile_thread_player_missiles[i].x - update_missile_thread_player_missiles[i].tar_x) < 1 && fabsf(update_missile_thread_player_missiles[i].y - update_missile_thread_player_missiles[i].tar_y) < 1) {
                    update_missile_thread_player_missiles[i].live = 0;
                    checkHitPlayer(game_screen, update_missile_thread_player_missiles[i].x, update_missile_thread_player_missiles[i].y);
                    killMissile(game_screen, &update_missile_thread_player_missiles[i]);
                }

                update_missile_thread_player_missiles[i].old_x = update_missile_thread_player_missiles[i].x;
                update_missile_thread_player_missiles[i].old_y = update_missile_thread_player_missiles[i].y;
                update_missile_thread_player_missiles[i].x += update_missile_thread_player_missiles[i].vel_x;
                update_missile_thread_player_missiles[i].y += update_missile_thread_player_missiles[i].vel_y;

                pthread_mutex_lock(&lock);
                wattron(game_screen, COLOR_PAIR(5));
                if (round(update_missile_thread_player_missiles[i].vel_x) > 0) {
                    mvwaddch(game_screen, round(update_missile_thread_player_missiles[i].old_y), round(update_missile_thread_player_missiles[i].old_x), '/');
                }
                else if (round(update_missile_thread_player_missiles[i].vel_x) < 0) {
                    mvwaddch(game_screen, round(update_missile_thread_player_missiles[i].old_y), round(update_missile_thread_player_missiles[i].old_x), '\\');
                } else {
                    mvwaddch(game_screen, round(update_missile_thread_player_missiles[i].old_y), round(update_missile_thread_player_missiles[i].old_x), '|');
                }
                wattron(game_screen, COLOR_PAIR(8));
                mvwaddch(game_screen, round(update_missile_thread_player_missiles[i].y), round(update_missile_thread_player_missiles[i].x), '.');
                pthread_mutex_unlock(&lock);
            }
        }
    }
    return NULL;
}

void *inputListener(void *arguments) {
    int input;
    int cur_x = 30;
    int cur_y = 10;
    wattron(game_screen, COLOR_PAIR(8));
    mvwaddch(game_screen, cur_y, cur_x, ACS_PLUS);
    while (main_loop_run) {
        input = wgetch(game_screen);
        switch (input) {
            case KEY_LEFT:
                pthread_mutex_lock(&lock);
                if (cur_x > 0) {
                    wattron(game_screen, COLOR_PAIR(8));
                    mvwaddch(game_screen, cur_y, cur_x, ' ');
                    cur_x -= 1;
                    mvwaddch(game_screen, cur_y, cur_x, ACS_PLUS);
                }
                pthread_mutex_unlock(&lock);
                break;
            case KEY_RIGHT:
                pthread_mutex_lock(&lock);
                if (cur_x < FRAME_WIDTH - 1) {
                    wattron(game_screen, COLOR_PAIR(8));
                    mvwaddch(game_screen, cur_y, cur_x, ' ');
                    cur_x += 1;
                    mvwaddch(game_screen, cur_y, cur_x, ACS_PLUS);
                }
                pthread_mutex_unlock(&lock);
                break;
            case KEY_UP:
                pthread_mutex_lock(&lock);
                if (cur_y > 0) {
                    wattron(game_screen, COLOR_PAIR(8));
                    mvwaddch(game_screen, cur_y, cur_x, ' ');
                    cur_y -= 1;
                    mvwaddch(game_screen, cur_y, cur_x, ACS_PLUS);
                }
                pthread_mutex_unlock(&lock);
                break;
            case KEY_DOWN:
                pthread_mutex_lock(&lock);
                if (cur_y < FRAME_HEIGHT - 7) {
                    wattron(game_screen, COLOR_PAIR(8));
                    mvwaddch(game_screen, cur_y, cur_x, ' ');
                    cur_y += 1;
                    mvwaddch(game_screen, cur_y, cur_x, ACS_PLUS);
                }
                pthread_mutex_unlock(&lock);
                break;
            case '1':
                shootPlayerMissile(update_missile_thread_player_missiles, cur_x, cur_y, 1);
                break;
            case '2':
                shootPlayerMissile(update_missile_thread_player_missiles, cur_x, cur_y, 2);
                break;
            case '3':
                shootPlayerMissile(update_missile_thread_player_missiles, cur_x, cur_y, 3);
                break;
            case 'q':
                update_missile_thread_live = 0;
                main_loop_run = 0;
                endwin();
                break;
        }
    }
    return NULL;
}

void game() {
    WINDOW *main_screen = newwin(FRAME_HEIGHT, FRAME_WIDTH, 0, 0);
    wattron(main_screen, A_BOLD);
    keypad(main_screen, TRUE);
    noecho();
    //nodelay(main_screen, TRUE);
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

    struct missile hostile_missiles[10] = {0};
    struct missile player_missiles[30] = {0};

    game_screen = main_screen;

    int base_x_pos[3] = {1, 56, 112};
    for (int i = 0; i < 3; i++) {
        bases[i] = (struct base) {
            .x = base_x_pos[i],
            .y = FRAME_HEIGHT - 6,
            .missile_count = 15
        };
        //mvwaddch(main_screen, bases[i].y, bases[i].x, 'X');
    }
    updateMissileCount();

    update_missile_thread_live = 1;
    update_missile_thread_hostile_missiles = hostile_missiles;
    update_missile_thread_player_missiles = player_missiles;
    pthread_t hostile_missiles_thread;
    pthread_create(&hostile_missiles_thread, NULL, updateHostileMissiles, NULL);

    pthread_t gen_hostile_missiles_thread;
    pthread_create(&gen_hostile_missiles_thread, NULL, genHostileMissiles, NULL);

    pthread_t input_listener_thread;
    pthread_create(&input_listener_thread, NULL, inputListener, NULL);

    //int main_loop_run = 1;

    while (main_loop_run) {
        usleep(10000);
        pthread_mutex_lock(&lock);
        wrefresh(main_screen);
        pthread_mutex_unlock(&lock);
    }
}
