// player.h
#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"

typedef struct {
    Entity entity;
} Player;

void InitPlayer(Player* player, float startX, float startY, float speed);

#endif // PLAYER_H
