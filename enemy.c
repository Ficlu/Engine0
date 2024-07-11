// enemy.c
#include "enemy.h"
#include "gameloop.h"
#include <stdlib.h>

void InitEnemy(Enemy* enemy, int startGridX, int startGridY, float speed) {
    enemy->entity.gridX = startGridX;
    enemy->entity.gridY = startGridY;
    enemy->entity.speed = speed;
    enemy->entity.posX = (2.0f * startGridX / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE);
    enemy->entity.posY = 1.0f - (2.0f * startGridY / GRID_SIZE) - (1.0f / GRID_SIZE);
    enemy->entity.targetGridX = startGridX;
    enemy->entity.targetGridY = startGridY;
    enemy->entity.needsPathfinding = false;
}

void MovementAI(Enemy* enemy) {
    // Simple random movement
    if (rand() % 100 < 5) {  // 5% chance to change direction
        enemy->entity.targetGridX = rand() % GRID_SIZE;
        enemy->entity.targetGridY = rand() % GRID_SIZE;
        enemy->entity.needsPathfinding = true;
    }
}