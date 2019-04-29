#ifndef GAME_H
#define GAME_H

#include <ncurses.h>

enum missileType {PLAYER, HOSTILE_NORMAL, HOSTILE_CRAZY};

void shoot_player_missile(int tar_x, int tar_y, int base);

void kill_missile(void *missile_input);

void update_missile_explosion(void *arguments);

void check_hit_player(float x, float y);

void updateHostileMissiles(void *arguments);

void inputListener(void *arguments);

void update_missile_count();

void game();

#endif
