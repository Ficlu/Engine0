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
    player->entity.finalGoalX = startGridX;
    player->entity.finalGoalY = startGridY;
    player->entity.needsPathfinding = false;
    player->entity.cachedPath = NULL;
    player->entity.cachedPathLength = 0;
    player->entity.currentPathIndex = 0;
    player->zoomFactor = 3.0f;
    player->entity.isPlayer = true;

    player->cameraTargetX = player->entity.posX;
    player->cameraTargetY = player->entity.posY;
    player->cameraCurrentX = player->entity.posX;
    player->cameraCurrentY = player->entity.posY;
    player->cameraSpeed = 0.1f;

    while (!isWalkable(player->entity.gridX, player->entity.gridY)) {
        player->entity.gridX = rand() % GRID_SIZE;
        player->entity.gridY = rand() % GRID_SIZE;
        player->entity.posX = (2.0f * player->entity.gridX / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE);
        player->entity.posY = 1.0f - (2.0f * player->entity.gridY / GRID_SIZE) - (1.0f / GRID_SIZE);
        player->cameraTargetX = player->entity.posX;
        player->cameraTargetY = player->entity.posY;
        player->cameraCurrentX = player->entity.posX;
        player->cameraCurrentY = player->entity.posY;
    }

    printf("Player initialized at (%d, %d)\n", player->entity.gridX, player->entity.gridY);
}

void UpdatePlayer(Player* player, Entity** allEntities, int entityCount) {
    UpdateEntity(&player->entity, allEntities, entityCount);

    float dx = player->entity.posX - player->cameraCurrentX;
    float dy = player->entity.posY - player->cameraCurrentY;
    float lookAheadFactor = 0.5f;

    player->lookAheadX = dx * lookAheadFactor;
    player->lookAheadY = dy * lookAheadFactor;

    player->cameraTargetX = player->entity.posX + player->lookAheadX;
    player->cameraTargetY = player->entity.posY + player->lookAheadY;

    float smoothFactor = 0.05f;
    player->cameraCurrentX += (player->cameraTargetX - player->cameraCurrentX) * smoothFactor;
    player->cameraCurrentY += (player->cameraTargetY - player->cameraCurrentY) * smoothFactor;
}

void CleanupPlayer(Player* player) {
    if (player->entity.cachedPath) {
        free(player->entity.cachedPath);
        player->entity.cachedPath = NULL;
    }
}
