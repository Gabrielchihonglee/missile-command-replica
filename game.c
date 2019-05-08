#include "game.h"
#include "end.h"
#include "functions.h"

#include "threading/thread.h"
#include "threading/input.h"
#include "threading/scheduler.h"
#include "threading/sleeper.h"

#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define MAX_HOSTILE_MISSILE 20
#define BASE_DEFAULT_MISSILE 10

struct missile {
    enum missileType type;
    int start_x, start_y;
    float x, y;
    float old_x, old_y;
    float vel_x, vel_y;
    int tar_x, tar_y;
    int live;
};

struct missile_explosion_thread_arg {
    WINDOW *screen;
    int x, y;
};

struct hostile_missiles_thread_arg {
    WINDOW *screen;
    int live;
    struct missile *hostile_missiles;
    struct missile *player_missiles;
};

struct base {
    int x, y;
    int missile_count;
    int live;
} bases[3];

static WINDOW *game_screen;
static int game_live = 0;
static struct missile hostile_missiles[MAX_HOSTILE_MISSILE] = {0};
static struct missile player_missiles[45] = {0};
static int level = 1;

char *STAGE_1, *STAGE_2;
char *LARGE_STAGE_1, *LARGE_STAGE_2;

// updates bases' missile count, as the name suggests
void update_missile_count() {
    char missile_count[3];
    for (int i = 0; i < 3; i++) {
        if (!bases[i].missile_count) {
            wattron(game_screen, COLOR_PAIR(84));
            mvwprintw(game_screen, bases[i].y + 4, bases[i].x + 4, "OUT");
        } else {
            sprintf(missile_count, "%i", bases[i].missile_count);
            wattron(game_screen, COLOR_PAIR(84));
            mvwprintw(game_screen, bases[i].y + 4, bases[i].x + 4, "   "); // first, erase previous number
            mvwprintw(game_screen, bases[i].y + 4, bases[i].x + 4, missile_count); // then, display new number
        }
    }
}

// activates player missile
void shoot_player_missile(int tar_x, int tar_y, int base) {
    if (!bases[base].missile_count) {
        update_missile_count();
        return;
    }
    wattron(game_screen, COLOR_PAIR(8));
    mvwaddch(game_screen, tar_y, tar_x, 'x'); // mark target position
    float dist;
    struct missile player_missile = {
        .type = PLAYER,
        .start_x =  bases[base].x + 4,
        .x = bases[base].x + 4,
        .start_y = FRAME_HEIGHT - 7,
        .y = FRAME_HEIGHT - 7,
        .vel_x = 0,
        .vel_y = 0,
        .tar_x = tar_x,
        .tar_y = tar_y,
        .live = 1
    };
    dist = sqrtf(powf(player_missile.x - player_missile.tar_x, 2) + powf(player_missile.y - player_missile.tar_y, 2));
    player_missile.vel_x = (player_missile.tar_x - player_missile.x) / dist * 2;
    player_missile.vel_y = (player_missile.tar_y - player_missile.y) / dist * 2;

    // add missile to the array of missiles to be updated
    for (int i = 0; i < 30; i++) {
        if (!player_missiles[i].live) {
            player_missiles[i] = player_missile;
            break;
        }
    }

    bases[base].missile_count--;
    update_missile_count();
}

// remove missile from screen
void kill_missile(void *missile_input) {
    struct missile *missile = missile_input;
    missile->live = 0;
    wattron(game_screen, COLOR_PAIR(1));
    //mvwaddch(game_screen, missile->tar_y, missile->tar_x, ' ');
    // clean trace from current position back to where it started
    while (1) {
        mvwaddch(game_screen, round(missile->y), round(missile->x), ' ');
        if (fabsf(missile->x - missile->start_x) < 0.1 && fabsf(missile->y - missile->start_y) < 0.1) {
            break;
        }
        // move backwards
        missile->x -= missile->vel_x;
        missile->y -= missile->vel_y;
    }
}

// handles missile explosion
void update_missile_explosion(void *arguments) {
    struct missile_explosion_thread_arg *args = arguments;

    sleep_add(0, 100000000);
    wattron(game_screen, COLOR_PAIR(8));
    draw_from_string(args->screen, args->x - 4, args->y - 2, LARGE_STAGE_1, DRAW); // explosion stage 1

    sleep_add(0, 100000000);
    wattron(game_screen, COLOR_PAIR(8));
    draw_from_string(args->screen, args->x - 4, args->y - 2, LARGE_STAGE_2, DRAW); // explosion stage 2

    sleep_add(0, 100000000);
    wattron(game_screen, COLOR_PAIR(8));
    draw_from_string(args->screen, args->x - 4, args->y - 2, LARGE_STAGE_2, ERASE); // clean up explosion

    free(arguments);
}

// starts the explosion
void init_missile_explosion(int x, int y) {
    struct missile_explosion_thread_arg *missile_explosion_args = malloc(sizeof(*missile_explosion_args));
    *missile_explosion_args = (struct missile_explosion_thread_arg) {
        .screen = game_screen,
        .x = x,
        .y = y
    };
    sched_wakeup(thread_create(&update_missile_explosion, missile_explosion_args));
}

// checks if hostile missile hit player's cities or bases
void check_hit_player(float x, float y) {
    // check city hit
    for (int i = 0; i < 6; i++) {
        if ((x >= cities_x_pos[i]) && (x <= (cities_x_pos[i] + 6))) {
            wattron(game_screen, COLOR_PAIR(3));
            // remove city from screen
            mvwprintw(game_screen, FRAME_HEIGHT - 4, cities_x_pos[i], "       ");
            mvwprintw(game_screen, FRAME_HEIGHT - 3, cities_x_pos[i], "       ");
            cities[i].live = 0;
        }
        init_missile_explosion(round(x), round(y));
    }
    // check base hit
    for (int i = 0; i < 3; i++) {
        if ((x >= bases[i].x) && (x <= (bases[i].x + 7))) {
            bases[i].missile_count = 0; // a destroyed base can't shoot missiles ya?
            bases[i].live = 0;
            draw_from_file(game_screen, bases[i].x, FRAME_HEIGHT - 6, "graphics/base", ERASE); // remove base from screen
            update_missile_count();
        }
        init_missile_explosion(round(x), round(y));
    }
}

// checks if player missile hit hostile missile
void check_hit_hostile(float x, float y) {
    for (int i = 0; i < MAX_HOSTILE_MISSILE; i++) {
        float dist = sqrt(pow((x - hostile_missiles[i].x), 2) + pow((y - hostile_missiles[i].y), 2));
        if (hostile_missiles[i].live && dist < 5) {
            kill_missile(&hostile_missiles[i]);
            score += score_multiplier(25, level);
        }
        init_missile_explosion(round(x), round(y));
    }
}

// generates hostile missiles
void gen_hostile_missiles() {
    int rand_target_type;
    int rand_target_x, rand_target_y;
    float dist;
    for (int i = 0; i < 8; i++) {
        // shoot 4 per wave, 5 seconds between 2 waves
        if (i == 4) {
            sleep_add(5, 0);
        }

        rand_target_type = rand() % 2 - 1; // half of the missiles will target cities, while others target bases
        if (rand_target_type) {
            rand_target_x = cities_x_pos[rand() % 6] + rand() % 4 + 1;
            rand_target_y = 36;
        } else {
            rand_target_x = bases_x_pos[rand() % 3] + rand() % 4 + 3;
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
        if (i == 2)
            hostile_missiles[i].type = HOSTILE_SPLIT;
        hostile_missiles[i].x = hostile_missiles[i].start_x;
        dist = sqrtf(powf(hostile_missiles[i].x - rand_target_x, 2) + rand_target_y * rand_target_y);
        hostile_missiles[i].vel_x = (rand_target_x - hostile_missiles[i].x) / dist;
        hostile_missiles[i].vel_y = rand_target_y / dist;
    }
}

void split_missile(struct missile *missile_pt) {
    missile_pt->type = HOSTILE_NORMAL;

    int rand_target_type;
    int rand_target_x, rand_target_y;
    float dist;

    rand_target_type = rand() % 2 - 1; // half of the missiles will target cities, while others target bases
    if (rand_target_type) {
        rand_target_x = cities_x_pos[rand() % 6] + rand() % 4 + 1;
        rand_target_y = 36;
    } else {
        rand_target_x = bases_x_pos[rand() % 3] + rand() % 4 + 3;
        rand_target_y = 33;
    }

    struct missile new_missile = (struct missile) {
        .live = 1,
        .type = HOSTILE_NORMAL,
        .start_x = missile_pt->x,
        .start_y = missile_pt->y,
        .x = missile_pt->x,
        .y = missile_pt->y,
        .vel_x = 0,
        .vel_y = 0,
        .tar_x = rand_target_x,
        .tar_y = rand_target_y
    };
    dist = sqrtf(powf(new_missile.x - rand_target_x, 2) + rand_target_y * rand_target_y);
    new_missile.vel_x = (rand_target_x - new_missile.x) / dist;
    new_missile.vel_y = (rand_target_y - new_missile.y) / dist;

    hostile_missiles[8] = new_missile;
}

// updates missiles position and prints the trail behind it
void sub_update_missiles(struct missile *missiles, int missile_count) {
    for (int i = 0; i < missile_count; i++) {
        if (missiles[i].live) {
            if (missiles[i].type == HOSTILE_SPLIT && fabsf(missiles[i].y - 10) < 1) {
                split_missile(&missiles[i]);
            }

            if (fabsf(missiles[i].x - missiles[i].tar_x) < 1 && fabsf(missiles[i].y - missiles[i].tar_y) < 1) { // check if it arrived at it's destination
                if (missiles[i].type == PLAYER)
                    check_hit_hostile(missiles[i].x, missiles[i].y);
                else
                    check_hit_player(missiles[i].x, missiles[i].y);
                kill_missile(&missiles[i]);
            } else {
                // calculate new posiiton of the missile
                missiles[i].old_x = missiles[i].x;
                missiles[i].old_y = missiles[i].y;
                missiles[i].x += missiles[i].vel_x;
                missiles[i].y += missiles[i].vel_y;
                // display the track behind the missile
                if (missiles[i].type == PLAYER) {
                    wattron(game_screen, COLOR_PAIR(5));
                    if (round(missiles[i].vel_x) > 0)
                        mvwaddch(game_screen, round(missiles[i].old_y), round(missiles[i].old_x), '/');
                    else if (round(player_missiles[i].vel_x) < 0)
                        mvwaddch(game_screen, round(missiles[i].old_y), round(missiles[i].old_x), '\\');
                    else
                        mvwaddch(game_screen, round(missiles[i].old_y), round(missiles[i].old_x), '|');
                } else {
                    wattron(game_screen, COLOR_PAIR(2));
                    if (round(missiles[i].vel_x) > 0)
                        mvwaddch(game_screen, round(missiles[i].old_y), round(missiles[i].old_x), '\\');
                    else if (round(missiles[i].vel_x) < 0)
                        mvwaddch(game_screen, round(missiles[i].old_y), round(missiles[i].old_x), '/');
                    else
                        mvwaddch(game_screen, round(missiles[i].old_y), round(missiles[i].old_x), '|');
                }
                // show position of missile
                wattron(game_screen, COLOR_PAIR(8));
                mvwaddch(game_screen, round(missiles[i].y), round(missiles[i].x), '.');
            }
        }
    }
}

// handles mechanic to move on to next level, actually, it's only 2 steps :P
void next_level() {
    level++;
    sched_wakeup(thread_create(&game, NULL));
}

// displays and runs the bonus points screen
void bonus_points() {
    wattron(game_screen, COLOR_PAIR(5));
    mvwprintw(game_screen, FRAME_HEIGHT / 2 - 2, FRAME_WIDTH / 2 - 6, "BONUS POINTS");

    // calculates and displays the bonus points for remaining player missiles
    int bonus_points_missiles = 0;
    char bonus_points_missiles_str[5];
    for (int i = 0; i < 3; i++)
        for (int j = bases[i].missile_count; j > 0; j--) {
            bases[i].missile_count -= 1; // remove missiles from bases as being counted
            bonus_points_missiles += 5; // 5 points per missile

            wattron(game_screen, COLOR_PAIR(2));
            sprintf(bonus_points_missiles_str, "%i", bonus_points_missiles);
            mvwprintw(game_screen, FRAME_HEIGHT / 2, FRAME_WIDTH / 2 - 6, bonus_points_missiles_str); // display count
            mvwaddch(game_screen, FRAME_HEIGHT / 2, FRAME_WIDTH / 2 - 2 + bonus_points_missiles / 5, '^'); // cute little display counting number of missiles left
            update_missile_count();

            wrefresh(game_screen);
            sleep_add(0, 100000000);
        }
    score += bonus_points_missiles;
    refresh_high_score(game_screen);
    wrefresh(game_screen);

    // calculates and displays the bonus points for remaining cities
    int bonus_points_cities = 0;
    char bonus_points_cities_str[5];
    for (int i = 0; i < 6; i++)
        if (cities[i].live) {
            // erase the cities upon being counted
            mvwprintw(game_screen, FRAME_HEIGHT - 4, cities_x_pos[i], "       ");
            mvwprintw(game_screen, FRAME_HEIGHT - 3, cities_x_pos[i], "       ");
            bonus_points_cities += 100; // 100 points per city

            wattron(game_screen, COLOR_PAIR(2));
            sprintf(bonus_points_cities_str, "%i", bonus_points_cities);
            mvwprintw(game_screen, FRAME_HEIGHT / 2 + 2, FRAME_WIDTH / 2 - 6, bonus_points_cities_str); // display counted

            // shows cute tiny version of the cities
            wattron(game_screen, COLOR_PAIR(3));
            mvwaddch(game_screen, FRAME_HEIGHT / 2 + 2, FRAME_WIDTH / 2 - 2 + (bonus_points_cities / 100) * 4 - 3, ACS_CKBOARD);
            wattron(game_screen, COLOR_PAIR(5));
            mvwaddch(game_screen, FRAME_HEIGHT / 2 + 2, FRAME_WIDTH / 2 - 2 + (bonus_points_cities / 100) * 4 + 1 - 3, ACS_CKBOARD);
            wattron(game_screen, COLOR_PAIR(3));
            mvwaddch(game_screen, FRAME_HEIGHT / 2 + 2, FRAME_WIDTH / 2 - 2 + (bonus_points_cities / 100) * 4 + 2 - 3, ACS_CKBOARD);

            wrefresh(game_screen);
            sleep_add(0, 300000000);
        }
    score += bonus_points_cities;
    refresh_high_score(game_screen);
    wrefresh(game_screen);

    sleep_add(1, 0);
}

// check if all hostile missiles are gone
void check_end_missiles() {
    int live_count = 0;
    for (int i = 0; i < MAX_HOSTILE_MISSILE; i++)
        if (hostile_missiles[i].live)
            live_count++;
    if (live_count == 0) {
        sleep_add(1, 0);
        game_live = 0;
        bonus_points();
        sleep_add(1, 0);
        endwin();
        //exit(0);
        next_level();
    }
}

// check if all cities are gone
void check_end_cities() {
    int live_count = 0;
    for (int i = 0; i < 6; i++)
        if (cities[i].live)
            live_count++;
    if (live_count == 0) {
        sleep_add(1, 0);
        game_live = 0;
        bonus_points();
        sleep_add(1, 0);
        endwin();
        end();
    }
}

// the game loop, as what the name of the function suggests
void game_loop() {
    int counter = 0; // game tick counter, min: 0; max: 4
    while (game_live) {
        sleep_add(0, 60000000 * 1 / pow(level * 0.06 + 0.95 ,2)); // speeds up the game as players progresses to higher levels
        if (counter < 4)
            counter++;
        else
            counter = 0;
        if (!counter) { // updates the hostile missiles 1 in 4 ticks, so that player missiles moves 4 times faster than hostile ones
            sub_update_missiles(hostile_missiles, MAX_HOSTILE_MISSILE);
            check_end_cities();
            check_end_missiles();
        }
        sub_update_missiles(player_missiles, 45);
        refresh_high_score(game_screen);
        wrefresh(game_screen);
    }
}

// moves the cursor from cur_x, cur_y to new_x, new_y
void move_cursor(int *cur_x, int *cur_y, int new_x, int new_y) {
    wattron(game_screen, COLOR_PAIR(5));
    if ((mvwinch(game_screen, *cur_y, *cur_x) & A_CHARTEXT) != ('x' & A_CHARTEXT)) { // don't erase if there's something besides of the cursor
        mvwaddch(game_screen, *cur_y, *cur_x, ' '); // erase old cursor
    }
    *cur_x = new_x;
    *cur_y = new_y;
    mvwaddch(game_screen, *cur_y, *cur_x, '+'); // draw cursor at new position
    wrefresh(game_screen);
}

// input handler for game screen
void game_screen_input() {
    int cur_x = FRAME_WIDTH / 2;
    int cur_y = (FRAME_HEIGHT - 7) / 2;
    wattron(game_screen, COLOR_PAIR(5));
    mvwaddch(game_screen, cur_y, cur_x, '+');
    int input;
    // MEVENT event;
    input_set_thread();
    while (game_live) {
        input_set_thread();
        input = wgetch(game_screen);
        switch (input) {
            /** case KEY_MOUSE:
                if (getmouse(&event) == OK) {
                    if (event.x < FRAME_WIDTH && event.y < (FRAME_HEIGHT - 7)) {
                        move_cursor(&cur_x, &cur_y, event.x, event.y);
                    }
                  }
                break; */
            case KEY_LEFT:
                if (cur_x > 0) {
                    move_cursor(&cur_x, &cur_y, cur_x - 1, cur_y);
                }
                break;
            case KEY_RIGHT:
                if (cur_x < FRAME_WIDTH - 1) {
                    move_cursor(&cur_x, &cur_y, cur_x + 1, cur_y);
                }
                break;
            case KEY_UP:
                if (cur_y > 0) {
                    move_cursor(&cur_x, &cur_y, cur_x, cur_y - 1);
                }
                break;
            case KEY_DOWN:
                if (cur_y < FRAME_HEIGHT - 7) {
                    move_cursor(&cur_x, &cur_y, cur_x, cur_y + 1);
                }
                break;
            case '1':
                shoot_player_missile(cur_x, cur_y, 0);
                break;
            case '2':
                shoot_player_missile(cur_x, cur_y, 1);
                break;
            case '3':
                shoot_player_missile(cur_x, cur_y, 2);
                break;
            case 'q':
                game_live = 0;
                endwin();
                exit(0);
                break;
            //default:
                //sched_wakeup(game_thread);
        }
    }
}

// introduces the stage to user
void stage_intro() {
    draw_screen_settings(game_screen, 0, cities);
    char level_text[5], score_x_text[5], speed_x_text[316];
    sprintf(level_text, "%i", level);
    sprintf(score_x_text, "%i", score_multiplier(1, level));
    sprintf(speed_x_text, "%.2f", pow(level * 0.06 + 0.95 ,2));
    char field[4][2][10] = {
        {"PLAYER", "1"},
        {"LEVEL", NULL},
        {"SCORE X", NULL},
        {"SPEED X", NULL}
    };
    strcpy(field[1][1], level_text);
    strcpy(field[2][1], score_x_text);
    strcpy(field[3][1], speed_x_text);
    for (int i = 0; i < 4; i++) {
        wattron(game_screen, COLOR_PAIR(5));
        mvwprintw(game_screen, FRAME_HEIGHT / 2 - 4 + i * 2, FRAME_WIDTH / 2 - 6, field[i][0]);
        wattron(game_screen, COLOR_PAIR(2));
        mvwprintw(game_screen, FRAME_HEIGHT / 2 - 4 + i * 2, FRAME_WIDTH / 2 + 3, field[i][1]);
    }
    wrefresh(game_screen);
    sleep_add(2, 0);
    werase(game_screen);
}

void game() {
    sched_wakeup(thread_create(&game_screen_input, NULL));

    LARGE_STAGE_1 = file_to_string("graphics/explosion-large-stage-1");
    LARGE_STAGE_2 = file_to_string("graphics/explosion-large-stage-2");

    game_screen = newwin(FRAME_HEIGHT, FRAME_WIDTH, 0, 0);
    game_live = 1;
    wattron(game_screen, A_BOLD);
    keypad(game_screen, TRUE);
    noecho();

    stage_intro();

    // setup all bases
    for (int i = 0; i < 3; i++) {
        bases[i] = (struct base) {
            .x = bases_x_pos[i],
            .y = FRAME_HEIGHT - 6,
            .missile_count = BASE_DEFAULT_MISSILE,
            .live = 1
        };
    }

    // setup all cities
    if (level == 1) // only on level #1
        for (int i = 0; i < 6; i++) {
            cities[i] = (struct city) {
                .x = cities_x_pos[i],
                .y = FRAME_HEIGHT - 4,
                .live = 1
            };
        }

    draw_screen_settings(game_screen, 0, cities);
    update_missile_count();
    wrefresh(game_screen);

    // let the 2 threads run the game
    sched_wakeup(thread_create(&gen_hostile_missiles, NULL));
    sched_wakeup(thread_create(&game_loop, NULL));
}
