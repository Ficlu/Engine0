// enemy.c
#include "enemy.h"
#include "gameloop.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "grid.h"

void InitEnemy(Enemy* enemy, float startX, float startY, float speed) {
    InitEntity(&enemy->entity, startX, startY, speed);
}

void UpdateEnemy(Enemy* enemy) {
    UpdateEntity(&enemy->entity);
}

void MovementAI(Enemy* enemy) {
    static int moveCounter = 0;
    moveCounter++;

    // Only update the path every 100 frames (adjust as needed)
    if (moveCounter % 100 == 0) {
        int targetGridX = rand() % GRID_SIZE;
        int targetGridY = rand() % GRID_SIZE;

        // Ensure the target position is walkable
        while (!isWalkable(targetGridX, targetGridY)) {
            targetGridX = rand() % GRID_SIZE;
            targetGridY = rand() % GRID_SIZE;
        }

        enemy->entity.targetPosX = (2.0f * targetGridX / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE);
        enemy->entity.targetPosY = 1.0f - (2.0f * targetGridY / GRID_SIZE) - (1.0f / GRID_SIZE);
        
        enemy->entity.needsPathfinding = true;
    }
}