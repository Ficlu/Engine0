/*
 * entity.c
 * 
 * This file contains the implementation of entity-related functions.
 * It handles entity movement, pathfinding, and updates.
 *
 * Copyright (c) 2024 Github user ficlu. All rights reserved.
 */

#include "entity.h"
#include "grid.h"
#include "gameloop.h"
#include "pathfinding.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <immintrin.h>
#include <stdatomic.h>

/*
 * sgn
 *
 * Returns the sign of a float value.
 *
 * @param[in] x The float value to determine the sign of
 * @return -1 if x is negative, 1 if x is positive, 0 if x is zero
 */
int sgn(float x) {
    return (x > 0) - (x < 0);
}

/*
 * UpdateEntity
 *
 * Update the entity's position and state.
 *
 * @param[in,out] entity Pointer to the Entity structure to update
 * @param[in] allEntities Array of pointers to all entities in the game
 * @param[in] entityCount Number of entities in the allEntities array
 *
 * @pre entity is a valid pointer to an Entity structure
 * @pre allEntities is a valid array of Entity pointers
 * @pre entityCount is a positive integer
 */
void UpdateEntity(Entity* entity, Entity** allEntities, int entityCount) {
    if (entity == NULL || allEntities == NULL) {
        fprintf(stderr, "Error: NULL pointer passed to UpdateEntity\n");
        return;
    }

    updateEntityPath(entity);

    int currentGridX = atomic_load(&entity->gridX);
    int currentGridY = atomic_load(&entity->gridY);
    float currentPosX = entity->posX;  // Direct read for _Atomic float
    float currentPosY = entity->posY;  // Direct read for _Atomic float
    int currentTargetGridX = atomic_load(&entity->targetGridX);
    int currentTargetGridY = atomic_load(&entity->targetGridY);

    float targetScreenX, targetScreenY;
    WorldToScreenCoords(currentTargetGridX, currentTargetGridY, 0, 0, 1, &targetScreenX, &targetScreenY);

    float dx = targetScreenX - currentPosX;
    float dy = targetScreenY - currentPosY;
    float distance = sqrt(dx*dx + dy*dy);

    if (distance < 0.001f) {
        entity->posX = targetScreenX;  // Direct write for _Atomic float
        entity->posY = targetScreenY;  // Direct write for _Atomic float
        atomic_store(&entity->gridX, currentTargetGridX);
        atomic_store(&entity->gridY, currentTargetGridY);
        atomic_store(&entity->needsPathfinding, true);
        return;
    }

    float moveDistance = fmin(entity->speed, distance);
    float moveX = (dx / distance) * moveDistance;
    float moveY = (dy / distance) * moveDistance;

    float newX = currentPosX + moveX;
    float newY = currentPosY + moveY;

    int newGridX = (int)((newX + 1.0f) * GRID_SIZE / 2);
    int newGridY = (int)((1.0f - newY) * GRID_SIZE / 2);

    bool canMove = true;

    if (newGridX != currentGridX && newGridY != currentGridY) {
        if (!isWalkable(newGridX, currentGridY) && !isWalkable(currentGridX, newGridY)) {
            canMove = false;
        }
    }

    if (canMove && !isWalkable(newGridX, newGridY)) {
        canMove = false;
    }

    if (canMove) {
        entity->posX = newX;  // Direct write for _Atomic float
        entity->posY = newY;  // Direct write for _Atomic float
        atomic_store(&entity->gridX, newGridX);
        atomic_store(&entity->gridY, newGridY);
    } else {
        atomic_store(&entity->needsPathfinding, true);
    }
}

/*
 * updateEntityPath
 *
 * Update the entity's path using A* pathfinding.
 *
 * @param[in,out] entity Pointer to the Entity structure to update
 *
 * @pre entity is a valid pointer to an Entity structure
 */
void updateEntityPath(Entity* entity) {
    if (entity == NULL) {
        fprintf(stderr, "Error: entity pointer is NULL in updateEntityPath\n");
        return;
    }

    int startX = atomic_load(&entity->gridX);
    int startY = atomic_load(&entity->gridY);
    int goalX = atomic_load(&entity->finalGoalX);
    int goalY = atomic_load(&entity->finalGoalY);

    if (startX == goalX && startY == goalY) {
        atomic_store(&entity->targetGridX, startX);
        atomic_store(&entity->targetGridY, startY);
        atomic_store(&entity->needsPathfinding, false);
        return;
    }

    int pathLength;
    Node* path = findPath(startX, startY, goalX, goalY, &pathLength);
    if (path) {
        // Free the old cached path
        if (entity->cachedPath) {
            free(entity->cachedPath);
        }

        entity->cachedPath = path;
        entity->cachedPathLength = pathLength;
        entity->currentPathIndex = 0;

        if (pathLength > 1) {
            atomic_store(&entity->targetGridX, entity->cachedPath[1].x);
            atomic_store(&entity->targetGridY, entity->cachedPath[1].y);
        } else {
            atomic_store(&entity->targetGridX, entity->cachedPath[0].x);
            atomic_store(&entity->targetGridY, entity->cachedPath[0].y);
        }
    } else {
        if (entity->cachedPath) {
            free(entity->cachedPath);
            entity->cachedPath = NULL;
        }
        // If no path is found, move towards the goal
        int dx = goalX - startX;
        int dy = goalY - startY;
        if (abs(dx) > abs(dy)) {
            atomic_store(&entity->targetGridX, startX + (dx > 0 ? 1 : -1));
            atomic_store(&entity->targetGridY, startY);
        } else {
            atomic_store(&entity->targetGridX, startX);
            atomic_store(&entity->targetGridY, startY + (dy > 0 ? 1 : -1));
        }
        if (!isWalkable(atomic_load(&entity->targetGridX), atomic_load(&entity->targetGridY))) {
            atomic_store(&entity->targetGridX, startX);
            atomic_store(&entity->targetGridY, startY);
        }
    }

    atomic_store(&entity->needsPathfinding, false);
}
/*
 * findNearestWalkableTile
 *
 * Find the nearest walkable tile to the given position.
 *
 * @param[in] posX X-coordinate of the starting position
 * @param[in] posY Y-coordinate of the starting position
 * @param[out] nearestX Pointer to store the X-coordinate of the nearest walkable tile
 * @param[out] nearestY Pointer to store the Y-coordinate of the nearest walkable tile
 *
 * @pre nearestX and nearestY are valid pointers
 */
void findNearestWalkableTile(float posX, float posY, int* nearestX, int* nearestY) {
    if (nearestX == NULL || nearestY == NULL) {
        fprintf(stderr, "Error: NULL pointer passed to findNearestWalkableTile\n");
        return;
    }

    int gridX = (int)((posX + 1.0f) * GRID_SIZE / 2);
    int gridY = (int)((1.0f - posY) * GRID_SIZE / 2);

    for (int radius = 0; radius < GRID_SIZE; radius++) {
        for (int dx = -radius; dx <= radius; dx++) {
            for (int dy = -radius; dy <= radius; dy++) {
                int nx = gridX + dx;
                int ny = gridY + dy;
                if (isWalkable(nx, ny)) {
                    *nearestX = nx;
                    *nearestY = ny;
                    return;
                }
            }
        }
    }
}