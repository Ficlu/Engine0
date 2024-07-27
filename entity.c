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

// Helper function to find the nearest walkable tile
void findNearestWalkableTile(float posX, float posY, int* nearestX, int* nearestY) {
    int centerX = (int)((posX + 1.0f) * GRID_SIZE / 2);
    int centerY = (int)((1.0f - posY) * GRID_SIZE / 2);
    
    int radius = 0;
    while (radius < GRID_SIZE) {  // Limit the search to avoid infinite loop
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                if (abs(dx) == radius || abs(dy) == radius) {  // Only check the perimeter
                    int checkX = centerX + dx;
                    int checkY = centerY + dy;
                    if (isValid(checkX, checkY) && isWalkable(checkX, checkY)) {
                        *nearestX = checkX;
                        *nearestY = checkY;
                        return;
                    }
                }
            }
        }
        radius++;
    }
    
    // If no walkable tile found, return the original position
    *nearestX = centerX;
    *nearestY = centerY;
}

void UpdateEntity(Entity* entity, Entity** allEntities, int entityCount) {
    updateEntityPath(entity);

    float targetPosX = (2.0f * entity->targetGridX / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE);
    float targetPosY = 1.0f - (2.0f * entity->targetGridY / GRID_SIZE) - (1.0f / GRID_SIZE);

    float dx = targetPosX - entity->posX;
    float dy = targetPosY - entity->posY;
    float distance = sqrt(dx*dx + dy*dy);

    if (entity->isPlayer) {
        printf("Player current position: (%f, %f) Grid: (%d, %d)\n", entity->posX, entity->posY, entity->gridX, entity->gridY);
        printf("Player target position: (%f, %f) Grid: (%d, %d)\n", targetPosX, targetPosY, entity->targetGridX, entity->targetGridY);
        printf("Distance to target: %f\n", distance);
    }

    // If very close to target, snap to it
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

    // Check if we're trying to move diagonally between unwalkable tiles
    if (newGridX != currentGridX && newGridY != currentGridY) {
        if (!isWalkable(newGridX, currentGridY) && !isWalkable(currentGridX, newGridY)) {
            canMove = false;
        }
    }

    // If not moving diagonally between unwalkable tiles, check the destination
    if (canMove && !isWalkable(newGridX, newGridY)) {
        canMove = false;
    }

    if (canMove) {
        entity->posX = newX;
        entity->posY = newY;
        entity->gridX = newGridX;
        entity->gridY = newGridY;
        
        if (entity->isPlayer) {
            printf("Player moved to: (%f, %f) Grid: (%d, %d)\n", entity->posX, entity->posY, entity->gridX, entity->gridY);
        }
    } else {
        // If can't move, stay at the current position
        entity->needsPathfinding = true;
        
        if (entity->isPlayer) {
            printf("Player couldn't move, stayed at: (%d, %d)\n", entity->gridX, entity->gridY);
        }
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

        // Reconstruct and store the path
        Node* current = &path[goalY * GRID_SIZE + goalX];
        while (current) {
            entity->cachedPathLength++;
            current = current->parent;
        }

        // Allocate memory for the path
        entity->cachedPath = (Node*)malloc(sizeof(Node) * entity->cachedPathLength);

        // Store the path in reverse order (from start to goal)
        current = &path[goalY * GRID_SIZE + goalX];
        for (int i = entity->cachedPathLength - 1; i >= 0; i--) {
            entity->cachedPath[i] = *current;
            current = current->parent;
        }

        entity->currentPathIndex = 0;

        // Set the next target
        if (entity->cachedPathLength > 1) {
            entity->targetGridX = entity->cachedPath[1].x;
            entity->targetGridY = entity->cachedPath[1].y;
        } else {
            entity->targetGridX = entity->cachedPath[0].x;
            entity->targetGridY = entity->cachedPath[0].y;
        }

        if (entity->isPlayer) {
            printf("Player: Path found. Length: %d, Next step: (%d, %d)\n", 
                   entity->cachedPathLength, entity->targetGridX, entity->targetGridY);
        }
    } else {
        if (entity->isPlayer) {
            printf("Player: No path found from (%d, %d) to (%d, %d)\n", startX, startY, goalX, goalY);
        }
        // If no path is found, move towards the goal one step at a time
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
