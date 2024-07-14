// enemy.c
#include "enemy.h"
#include "gameloop.h"
#include <stdlib.h>
#include <stdio.h>

void InitEnemy(Enemy* enemy, int startGridX, int startGridY, float speed) {
    enemy->entity.gridX = startGridX;
    enemy->entity.gridY = startGridY;
    enemy->entity.speed = speed;
    enemy->entity.posX = (2.0f * startGridX / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE);
    enemy->entity.posY = 1.0f - (2.0f * startGridY / GRID_SIZE) - (1.0f / GRID_SIZE);
    enemy->entity.targetGridX = startGridX;
    enemy->entity.targetGridY = startGridY;
    enemy->entity.needsPathfinding = false;
    enemy->entity.currentPath = NULL;
    enemy->entity.pathLength = 0;
    printf("Enemy initialized at (%d, %d)\n", startGridX, startGridY);
}

void MovementAI(Enemy* enemy) {
    // Simple random movement
    if (rand() % 100 < 5) {  // 5% chance to change direction
        int newTargetX, newTargetY;
        do {
            newTargetX = rand() % GRID_SIZE;
            newTargetY = rand() % GRID_SIZE;
        } while (!isWalkable(newTargetX, newTargetY));

        enemy->entity.targetGridX = newTargetX;
        enemy->entity.targetGridY = newTargetY;
        enemy->entity.needsPathfinding = true;
        printf("Enemy at (%d, %d) set new target: (%d, %d)\n", 
               enemy->entity.gridX, enemy->entity.gridY, 
               enemy->entity.targetGridX, enemy->entity.targetGridY);
    }
}

void UpdateEnemy(Enemy* enemy, Entity** allEntities, int entityCount) {
    MovementAI(enemy);
    UpdateEntity(&enemy->entity, allEntities, entityCount);
}

void CleanupEnemy(Enemy* enemy) {
    if (enemy->entity.currentPath) {
        free(enemy->entity.currentPath);
        enemy->entity.currentPath = NULL;
        enemy->entity.pathLength = 0;
    }
}