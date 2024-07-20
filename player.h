#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"

typedef struct {
    Entity entity;
    float cameraOffsetX;
    float cameraOffsetY;
} Player;

void InitPlayer(Player* player, int startGridX, int startGridY, float speed);
void UpdatePlayer(Player* player, Entity** allEntities, int entityCount);
void CleanupPlayer(Player* player);

#endif // PLAYER_H
