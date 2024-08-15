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

    enemy->entity.gridX = startGridX;
    enemy->entity.gridY = startGridY;
    enemy->entity.speed = speed;
    WorldToScreenCoords(startGridX, startGridY, 0, 0, 1, &enemy->entity.posX, &enemy->entity.posY);
    enemy->entity.targetGridX = startGridX;
    enemy->entity.targetGridY = startGridX;
    enemy->entity.finalGoalX = startGridX;
    enemy->entity.finalGoalY = startGridY;
    enemy->entity.needsPathfinding = false;
    enemy->entity.cachedPath = NULL;
    enemy->entity.cachedPathLength = 0;
    enemy->entity.currentPathIndex = 0;
    enemy->entity.isPlayer = false;

    findNearestWalkableTile(enemy->entity.posX, enemy->entity.posY, &enemy->entity.gridX, &enemy->entity.gridY);
    WorldToScreenCoords(enemy->entity.gridX, enemy->entity.gridY, 0, 0, 1, &enemy->entity.posX, &enemy->entity.posY);

    enemy->pathColor.r = (float)rand() / RAND_MAX;
    enemy->pathColor.g = (float)rand() / RAND_MAX;
    enemy->pathColor.b = (float)rand() / RAND_MAX;

    printf("Enemy initialized at (%d, %d) with path color (%f, %f, %f)\n", 
           enemy->entity.gridX, enemy->entity.gridY, enemy->pathColor.r, enemy->pathColor.g, enemy->pathColor.b);
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
void MovementAI(Enemy* enemy) {
    if (enemy == NULL) {
        fprintf(stderr, "Error: enemy pointer is NULL in MovementAI\n");
        return;
    }

    if ((enemy->entity.gridX == enemy->entity.targetGridX &&
         enemy->entity.gridY == enemy->entity.targetGridY) ||
        enemy->entity.needsPathfinding) {
        
        /* Only change path with a 30% chance */
        if (rand() % 100 < 30) {
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
            enemy->entity.finalGoalX = enemy->entity.gridX;
            enemy->entity.finalGoalY = enemy->entity.gridY;
            enemy->entity.needsPathfinding = false;
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
void UpdateEnemy(Enemy* enemy, Entity** allEntities, int entityCount) {
    if (enemy == NULL || allEntities == NULL) {
        fprintf(stderr, "Error: NULL pointer passed to UpdateEnemy\n");
        return;
    }

    MovementAI(enemy);
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