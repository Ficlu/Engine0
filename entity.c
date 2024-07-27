// entity.c
#include "entity.h"
#include "grid.h"
#include "gameloop.h"
#include "pathfinding.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <immintrin.h>

int sgn(float x) {
    return (x > 0) - (x < 0);
}

void UpdateEntity(Entity* entity, Entity** allEntities, int entityCount) {
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

void updateEntityPath(Entity* entity) {
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

        entity->cachedPath = (Node*)malloc(sizeof(Node) * entity->cachedPathLength);

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
