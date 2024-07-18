#include "enemy.h"
#include "gameloop.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "enemy.h"
#include "gameloop.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

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
    enemy->entity.cachedPath = NULL;
    enemy->entity.cachedPathLength = 0;
    enemy->entity.currentPathIndex = 0;
    enemy->entity.isPlayer = false;

    // Ensure the enemy starts on a walkable tile
    while (!isWalkable(enemy->entity.gridX, enemy->entity.gridY)) {
        enemy->entity.gridX = rand() % GRID_SIZE;
        enemy->entity.gridY = rand() % GRID_SIZE;
        enemy->entity.posX = (2.0f * enemy->entity.gridX / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE);
        enemy->entity.posY = 1.0f - (2.0f * enemy->entity.gridY / GRID_SIZE) - (1.0f / GRID_SIZE);
    }

    // Initialize random color for the enemy path
    enemy->pathColor.r = (float)rand() / RAND_MAX;
    enemy->pathColor.g = (float)rand() / RAND_MAX;
    enemy->pathColor.b = (float)rand() / RAND_MAX;

    printf("Enemy initialized at (%d, %d) with path color (%f, %f, %f)\n", 
           enemy->entity.gridX, enemy->entity.gridY, enemy->pathColor.r, enemy->pathColor.g, enemy->pathColor.b);
}

void MovementAI(Enemy* enemy) {
    // Check if the enemy has reached its current target or needs a new target
if ((enemy->entity.gridX == enemy->entity.targetGridX &&
     enemy->entity.gridY == enemy->entity.targetGridY) ||
    enemy->entity.needsPathfinding) {
        
        // Only change path with a 30% chance
        if (rand() % 100 < 30) {
            // Choose a new random target
            int newTargetX, newTargetY;
            do {
                newTargetX = rand() % GRID_SIZE;
                newTargetY = rand() % GRID_SIZE;
            } while (!isWalkable(newTargetX, newTargetY));

            enemy->entity.finalGoalX = newTargetX;
            enemy->entity.finalGoalY = newTargetY;
            enemy->entity.needsPathfinding = true;
            printf("Enemy at (%d, %d) set new target: (%d, %d)\n", 
                   enemy->entity.gridX, enemy->entity.gridY, 
                   enemy->entity.finalGoalX, enemy->entity.finalGoalY);
        } else {
            // If not changing path, just set the current position as the target
            enemy->entity.finalGoalX = enemy->entity.gridX;
            enemy->entity.finalGoalY = enemy->entity.gridY;
            enemy->entity.needsPathfinding = false;
        }
    }
}

void UpdateEnemy(Enemy* enemy, Entity** allEntities, int entityCount) {
    MovementAI(enemy);
    UpdateEntity(&enemy->entity, allEntities, entityCount);
    
    // If the enemy has finished its current path, trigger a new target selection
    if (enemy->entity.currentPathIndex >= enemy->entity.cachedPathLength) {
        enemy->entity.needsPathfinding = true;
    }
}

void CleanupEnemy(Enemy* enemy) {
    if (enemy->entity.cachedPath) {
        free(enemy->entity.cachedPath);
        enemy->entity.cachedPath = NULL;
    }
}