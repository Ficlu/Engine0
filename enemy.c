// enemy.c
#include "enemy.h"
#include "gameloop.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

void InitEnemy(Enemy* enemy, int startGridX, int startGridY, float speed) {
    enemy->entity.gridX = startGridX;
    enemy->entity.gridY = startGridY;
    enemy->entity.speed = speed;
    enemy->entity.posX = (2.0f * startGridX / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE);
    enemy->entity.posY = 1.0f - (2.0f * startGridY / GRID_SIZE) - (1.0f / GRID_SIZE);
    enemy->entity.targetGridX = startGridX;
    enemy->entity.targetGridY = startGridY;
    enemy->entity.finalGoalX = startGridX;
    enemy->entity.finalGoalY = startGridY;
    enemy->entity.needsPathfinding = false;
    enemy->entity.currentPath = NULL;
    enemy->entity.pathLength = 0;
    enemy->entity.isPlayer = false;
    printf("Enemy initialized at (%d, %d)\n", startGridX, startGridY);
}

void MovementAI(Enemy* enemy) {
    // Check if the enemy has reached its target
    float targetPosX = (2.0f * enemy->entity.targetGridX / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE);
    float targetPosY = 1.0f - (2.0f * enemy->entity.targetGridY / GRID_SIZE) - (1.0f / GRID_SIZE);
    
    float dx = targetPosX - enemy->entity.posX;
    float dy = targetPosY - enemy->entity.posY;
    float distance = sqrt(dx*dx + dy*dy);

    if (distance < 0.001f || (enemy->entity.gridX == enemy->entity.targetGridX && enemy->entity.gridY == enemy->entity.targetGridY)) {
        // If at the target, potentially choose a new target
        if (rand() % 100 < 3) {  // 3% chance to change direction
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
}

void UpdateEnemy(Enemy* enemy, Entity** allEntities, int entityCount) {
    MovementAI(enemy);
    UpdateEntity(&enemy->entity, allEntities, entityCount);

    // Additional check to ensure the enemy is on a walkable tile
    if (!isWalkable(enemy->entity.gridX, enemy->entity.gridY)) {
        // Find the nearest walkable tile
        int nearestX = enemy->entity.gridX;
        int nearestY = enemy->entity.gridY;
        float minDistance = INFINITY;

        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int checkX = enemy->entity.gridX + dx;
                int checkY = enemy->entity.gridY + dy;
                if (checkX >= 0 && checkX < GRID_SIZE && checkY >= 0 && checkY < GRID_SIZE && isWalkable(checkX, checkY)) {
                    float dist = dx*dx + dy*dy;
                    if (dist < minDistance) {
                        minDistance = dist;
                        nearestX = checkX;
                        nearestY = checkY;
                    }
                }
            }
        }

        // Snap to the nearest walkable tile
        enemy->entity.gridX = nearestX;
        enemy->entity.gridY = nearestY;
        enemy->entity.posX = (2.0f * nearestX / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE);
        enemy->entity.posY = 1.0f - (2.0f * nearestY / GRID_SIZE) - (1.0f / GRID_SIZE);

        // Set the current position as the new target and request pathfinding
        enemy->entity.targetGridX = nearestX;
        enemy->entity.targetGridY = nearestY;
        enemy->entity.needsPathfinding = true;

        printf("Enemy snapped to nearest walkable tile: (%d, %d)\n", nearestX, nearestY);
    }
}


void CleanupEnemy(Enemy* enemy) {
    if (enemy->entity.currentPath) {
        free(enemy->entity.currentPath);
        enemy->entity.currentPath = NULL;
        enemy->entity.pathLength = 0;
    }
}