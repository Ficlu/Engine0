// player.h
#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"

typedef struct {
    Entity entity;
} Player;

void InitPlayer(Player* player, int tileX, int tileY, float speed);
void MovePlayer(Player* player, float dx, float dy);

#endif // PLAYER_H