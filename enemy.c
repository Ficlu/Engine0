/*
 * enemy.c
 * 
 * This file contains the implementation of enemy-related functions.
 * It handles enemy initialization, movement AI, and updates.
 *
 * Copyright (c) 2024 Github user ficlu. All rights reserved.
 */

#include "enemy.h"
#include "gameloop.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>


/*
 * InitEnemy
 *
 * Initialize an enemy entity with given starting position and speed.
 *
 * @param[out] enemy Pointer to the Enemy structure to initialize
 * @param[in] startGridX Starting X position on the grid
 * @param[in] startGridY Starting Y position on the grid
 * @param[in] speed Movement speed of the enemy
 *
 * @pre enemy is a valid pointer to an Enemy structure
 * @pre startGridX and startGridY are within valid grid bounds
 * @pre speed is a positive float value
 */
void InitEnemy(Enemy* enemy, int startGridX, int startGridY, float speed) {
    if (enemy == NULL) {
        fprintf(stderr, "Error: enemy pointer is NULL in InitEnemy\n");
        return;
    }

    atomic_store(&enemy->entity.gridX, startGridX);
    atomic_store(&enemy->entity.gridY, startGridY);
    enemy->entity.speed = speed;
    
    float tempPosX, tempPosY;
    WorldToScreenCoords(startGridX, startGridY, 0, 0, 1, &tempPosX, &tempPosY);
    atomic_store(&enemy->entity.posX, tempPosX);
    atomic_store(&enemy->entity.posY, tempPosY);

    atomic_store(&enemy->entity.targetGridX, startGridX);
    atomic_store(&enemy->entity.targetGridY, startGridY);
    atomic_store(&enemy->entity.finalGoalX, startGridX);
    atomic_store(&enemy->entity.finalGoalY, startGridY);
    atomic_store(&enemy->entity.needsPathfinding, false);
    enemy->entity.cachedPath = NULL;
    enemy->entity.cachedPathLength = 0;
    enemy->entity.currentPathIndex = 0;
    enemy->entity.isPlayer = false;

    int tempNearestX, tempNearestY;
    int attempts = 0;
    const int MAX_ATTEMPTS = 100;

    do {
        findNearestWalkableTile(atomic_load(&enemy->entity.posX), 
                               atomic_load(&enemy->entity.posY), 
                               &tempNearestX, &tempNearestY);
        attempts++;
        
        if (attempts >= MAX_ATTEMPTS) {
            fprintf(stderr, "Warning: Could not find valid walkable tile for enemy after %d attempts\n", 
                    MAX_ATTEMPTS);
            // Fall back to original position
            tempNearestX = startGridX;
            tempNearestY = startGridY;
            break;
        }
    } while (!isWalkable(tempNearestX, tempNearestY));

    atomic_store(&enemy->entity.gridX, tempNearestX);
    atomic_store(&enemy->entity.gridY, tempNearestY);

    WorldToScreenCoords(atomic_load(&enemy->entity.gridX), 
                       atomic_load(&enemy->entity.gridY), 
                       0, 0, 1, &tempPosX, &tempPosY);
    atomic_store(&enemy->entity.posX, tempPosX);
    atomic_store(&enemy->entity.posY, tempPosY);

    enemy->lastPathfindingTime = 0;
}
/*
 * MovementAI
 *
 * Implement basic movement AI for the enemy.
 *
 * @param[in,out] enemy Pointer to the Enemy structure to update
 *
 * @pre enemy is a valid pointer to an Enemy structure
 */

void MovementAI(Enemy* enemy, Uint32 currentTime) {
    if (enemy == NULL) {
        fprintf(stderr, "Error: enemy pointer is NULL in MovementAI\n");
        return;
    }

    if ((enemy->entity.gridX == enemy->entity.targetGridX &&
         enemy->entity.gridY == enemy->entity.targetGridY) ||
        enemy->entity.needsPathfinding) {
        
        /* Only change path with a 20% chance */
        if (rand() % 10 < 2) {
            int newTargetX, newTargetY;
            do {
                newTargetX = rand() % GRID_SIZE;
                newTargetY = rand() % GRID_SIZE;
            } while (!isWalkable(newTargetX, newTargetY));

            // Only trigger pathfinding if the goal has actually changed
            if (newTargetX != enemy->entity.finalGoalX || newTargetY != enemy->entity.finalGoalY) {
                enemy->entity.finalGoalX = newTargetX;
                enemy->entity.finalGoalY = newTargetY;
                enemy->entity.needsPathfinding = true;
                enemy->lastPathfindingTime = currentTime;
                //printf("Enemy at (%d, %d) set new target: (%d, %d)\n", 
                //       enemy->entity.gridX, enemy->entity.gridY, 
                //       enemy->entity.finalGoalX, enemy->entity.finalGoalY);
            }
        } else if (enemy->entity.needsPathfinding) {
            // If we're not changing the path, but pathfinding is needed (e.g., reached end of current path)
            enemy->lastPathfindingTime = currentTime;
        }
    }
}

/*
 * UpdateEnemy
 *
 * Update the enemy's state, including movement and pathfinding.
 *
 * @param[in,out] enemy Pointer to the Enemy structure to update
 * @param[in] allEntities Array of pointers to all entities in the game
 * @param[in] entityCount Number of entities in the allEntities array
 *
 * @pre enemy is a valid pointer to an Enemy structure
 * @pre allEntities is a valid array of Entity pointers
 * @pre entityCount is a positive integer
 */

void UpdateEnemy(Enemy* enemy, Entity** allEntities, int entityCount, Uint32 currentTime) {
    if (enemy == NULL || allEntities == NULL) {
        fprintf(stderr, "Error: NULL pointer passed to UpdateEnemy\n");
        return;
    }

    MovementAI(enemy, currentTime);
    
    if (enemy->entity.needsPathfinding) {
        // Check if the current path is still valid before recalculating
        bool pathStillValid = false;
        if (enemy->entity.cachedPath && enemy->entity.cachedPathLength > enemy->entity.currentPathIndex) {
            int nextX = enemy->entity.cachedPath[enemy->entity.currentPathIndex].x;
            int nextY = enemy->entity.cachedPath[enemy->entity.currentPathIndex].y;
            if (isWalkable(nextX, nextY)) {
                pathStillValid = true;
            }
        }

        if (!pathStillValid) {
            int pathLength;
            Node* newPath = findPathGPU(enemy->entity.gridX, enemy->entity.gridY, 
                                        enemy->entity.finalGoalX, enemy->entity.finalGoalY, 
                                        &pathLength);
            
            if (newPath) {
                if (enemy->entity.cachedPath) {
                    free(enemy->entity.cachedPath);
                }
                
                enemy->entity.cachedPath = newPath;
                enemy->entity.cachedPathLength = pathLength;
                enemy->entity.currentPathIndex = 0;
                enemy->entity.needsPathfinding = false;
                
                if (pathLength > 1) {
                    enemy->entity.targetGridX = newPath[1].x;
                    enemy->entity.targetGridY = newPath[1].y;
                } else {
                    enemy->entity.targetGridX = enemy->entity.gridX;
                    enemy->entity.targetGridY = enemy->entity.gridY;
                }
            } else {
                enemy->entity.targetGridX = enemy->entity.gridX;
                enemy->entity.targetGridY = enemy->entity.gridY;
                enemy->entity.needsPathfinding = false;
            }
        }
    }
    
    UpdateEntity(&enemy->entity, allEntities, entityCount);
    
    if (enemy->entity.currentPathIndex >= enemy->entity.cachedPathLength) {
        enemy->entity.needsPathfinding = true;
    }
}
/*
 * CleanupEnemy
 *
 * Free any dynamically allocated memory associated with the enemy.
 *
 * @param[in,out] enemy Pointer to the Enemy structure to clean up
 *
 * @pre enemy is a valid pointer to an Enemy structure
 */
void CleanupEnemy(Enemy* enemy) {
    if (enemy == NULL) {
        fprintf(stderr, "Error: enemy pointer is NULL in CleanupEnemy\n");
        return;
    }

    /* CERT C MEM31-C: Free dynamically allocated memory when no longer needed */
    if (enemy->entity.cachedPath) {
        free(enemy->entity.cachedPath);
        enemy->entity.cachedPath = NULL;
    }
}