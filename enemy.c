#include "enemy.h"
#include "gameloop.h"  // Ensure GRID_SIZE is available
#include <stdlib.h>

void InitEnemy(Enemy* enemy, float startX, float startY, float speed) {
    InitEntity(&enemy->entity, startX, startY, speed);
}

void UpdateEnemy(Enemy* enemy) {
    UpdateEntity(&enemy->entity);
}

void MovementAI(Enemy* enemy) {
    // Randomly change target position within the grid
    int targetGridX = rand() % GRID_SIZE;
    int targetGridY = rand() % GRID_SIZE;

    // Ensure the target position is within the grid bounds
    enemy->entity.targetPosX = (2.0f * targetGridX / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE);
    enemy->entity.targetPosY = 1.0f - (2.0f * targetGridY / GRID_SIZE) - (1.0f / GRID_SIZE);
}
