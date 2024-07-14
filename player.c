// player.c
#include "player.h"
#include "gameloop.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

void InitPlayer(Player* player, int startGridX, int startGridY, float speed) {
    player->entity.gridX = startGridX;
    player->entity.gridY = startGridY;
    player->entity.speed = speed;
    player->entity.posX = (2.0f * startGridX / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE);
    player->entity.posY = 1.0f - (2.0f * startGridY / GRID_SIZE) - (1.0f / GRID_SIZE);
    player->entity.targetGridX = startGridX;
    player->entity.targetGridY = startGridY;
    player->entity.needsPathfinding = false;
    player->entity.currentPath = NULL;
    player->entity.pathLength = 0;
    printf("Player initialized at (%d, %d)\n", startGridX, startGridY);
}

void UpdatePlayer(Player* player, Entity** allEntities, int entityCount) {
    UpdateEntity(&player->entity, allEntities, entityCount);
    printf("Player updated. Position: (%d, %d), Target: (%d, %d)\n", 
           player->entity.gridX, player->entity.gridY, 
           player->entity.targetGridX, player->entity.targetGridY);
}

void CleanupPlayer(Player* player) {
    if (player->entity.currentPath) {
        free(player->entity.currentPath);
        player->entity.currentPath = NULL;
        player->entity.pathLength = 0;
    }
}