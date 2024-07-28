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

    float targetPosX = (2.0f * entity->targetGridX / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE);
    float targetPosY = 1.0f - (2.0f * entity->targetGridY / GRID_SIZE) - (1.0f / GRID_SIZE);

    float dx = targetPosX - entity->posX;
    float dy = targetPosY - entity->posY;
    float distance = sqrt(dx*dx + dy*dy);

    if (distance < 0.001f) {
        entity->posX = targetPosX;
        entity->posY = targetPosY;
        entity->gridX = entity->targetGridX;
        entity->gridY = entity->targetGridY;
        entity->needsPathfinding = true;
        return;
    }

    float moveDistance = fmin(entity->speed, distance);
    float moveX = (dx / distance) * moveDistance;
    float moveY = (dy / distance) * moveDistance;

    float newX = entity->posX + moveX;
    float newY = entity->posY + moveY;

    int currentGridX = entity->gridX;
    int currentGridY = entity->gridY;
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
        entity->posX = newX;
        entity->posY = newY;
        entity->gridX = newGridX;
        entity->gridY = newGridY;
    } else {
        entity->needsPathfinding = true;
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

    int startX = entity->gridX;
    int startY = entity->gridY;
    int goalX = entity->finalGoalX;
    int goalY = entity->finalGoalY;

    if (startX == goalX && startY == goalY) {
        entity->targetGridX = startX;
        entity->targetGridY = startY;
        entity->needsPathfinding = false;
        return;
    }

    Node* path = findPath(startX, startY, goalX, goalY);
    if (path) {
        if (entity->cachedPath) {
            free(entity->cachedPath);
        }
        entity->cachedPath = path;
        entity->cachedPathLength = 0;

        Node* current = &path[goalY * GRID_SIZE + goalX];
        while (current) {
            entity->cachedPathLength++;
            current = current->parent;
        }

        /* CERT C ARR01-C: Do not apply the sizeof operator to a pointer when taking the size of an array */
        entity->cachedPath = (Node*)malloc(sizeof(Node) * entity->cachedPathLength);
        if (entity->cachedPath == NULL) {
            fprintf(stderr, "Error: Failed to allocate memory for cached path\n");
            return;
        }

        current = &path[goalY * GRID_SIZE + goalX];
        for (int i = entity->cachedPathLength - 1; i >= 0; i--) {
            entity->cachedPath[i] = *current;
            current = current->parent;
        }

        entity->currentPathIndex = 0;

        if (entity->cachedPathLength > 1) {
            entity->targetGridX = entity->cachedPath[1].x;
            entity->targetGridY = entity->cachedPath[1].y;
        } else {
            entity->targetGridX = entity->cachedPath[0].x;
            entity->targetGridY = entity->cachedPath[0].y;
        }
    } else {
        int dx = goalX - startX;
        int dy = goalY - startY;
        if (abs(dx) > abs(dy)) {
            entity->targetGridX = startX + (dx > 0 ? 1 : -1);
            entity->targetGridY = startY;
        } else {
            entity->targetGridX = startX;
            entity->targetGridY = startY + (dy > 0 ? 1 : -1);
        }
        if (!isWalkable(entity->targetGridX, entity->targetGridY)) {
            entity->targetGridX = startX;
            entity->targetGridY = startY;
        }
    }

    entity->needsPathfinding = false;
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