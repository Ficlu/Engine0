// player.c
#include "player.h"
#include "gameloop.h"
#include <math.h>

void InitPlayer(Player* player, int startGridX, int startGridY, float speed) {
    player->entity.gridX = startGridX;
    player->entity.gridY = startGridY;
    player->entity.speed = speed;
    player->entity.posX = (2.0f * startGridX / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE);
    player->entity.posY = 1.0f - (2.0f * startGridY / GRID_SIZE) - (1.0f / GRID_SIZE);
    player->entity.targetGridX = startGridX;
    player->entity.targetGridY = startGridY;
    player->entity.needsPathfinding = false;
}

void MovePlayer(Player* player, float dx, float dy) {
    // Move the player
    player->entity.posX += dx;
    player->entity.posY += dy;

    // Calculate the current grid position
    int currentGridX = (int)((player->entity.posX + 1.0f) * GRID_SIZE / 2);
    int currentGridY = (int)((1.0f - player->entity.posY) * GRID_SIZE / 2);

    // Calculate the center of the current grid cell
    float gridCenterX = (2.0f * currentGridX / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE);
    float gridCenterY = 1.0f - (2.0f * currentGridY / GRID_SIZE) - (1.0f / GRID_SIZE);

    // If the player is close to the center of a grid cell, snap to it
    float snapThreshold = 0.001f;  // Adjust as needed
    if (fabs(player->entity.posX - gridCenterX) < snapThreshold &&
        fabs(player->entity.posY - gridCenterY) < snapThreshold) {
        player->entity.posX = gridCenterX;
        player->entity.posY = gridCenterY;
        player->entity.gridX = currentGridX;
        player->entity.gridY = currentGridY;
    }
}