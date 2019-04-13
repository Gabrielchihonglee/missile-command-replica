#ifndef GAME_H
#define GAME_H

#include <ncurses.h>

void shootPlayerMissile(void *player_missiless, int tar_x, int tar_y, int base);

void killMissile(WINDOW *screen, void *missile_input);

void *updateMissileExplosion(void *arguments);

void checkHitPlayer(WINDOW *screen, float x, float y);

void *updateHostileMissiles(void *arguments);

void *inputListener(void *arguments);

void updateMissileCount();

void game();

#endif
