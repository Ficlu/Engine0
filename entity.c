#include "entity.h"
#include "grid.h"
#include "gameloop.h"
#include "pathfinding.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

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

    // If very close to target, snap to it (or to the nearest walkable tile)
    if (distance < 0.001f) {
        int snapX, snapY;
        findNearestWalkableTile(targetPosX, targetPosY, &snapX, &snapY);
        
        entity->posX = (2.0f * snapX / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE);
        entity->posY = 1.0f - (2.0f * snapY / GRID_SIZE) - (1.0f / GRID_SIZE);
        entity->gridX = snapX;
        entity->gridY = snapY;
        entity->needsPathfinding = true;
        
        if (entity->isPlayer) {
            printf("Player snapped to (%d, %d)\n", entity->gridX, entity->gridY);
        }
        return;
    }

    float moveDistance = fmin(entity->speed, distance);
    float moveX = (dx / distance) * moveDistance;
    float moveY = (dy / distance) * moveDistance;

    if (entity->isPlayer) {
        printf("Attempted move: (%f, %f)\n", moveX, moveY);
    }

    float newX = entity->posX + moveX;
    float newY = entity->posY + moveY;

    // Check if we've reached or passed the target
    if ((dx > 0 && newX >= targetPosX) || (dx < 0 && newX <= targetPosX)) {
        newX = targetPosX;
    }
    if ((dy > 0 && newY >= targetPosY) || (dy < 0 && newY <= targetPosY)) {
        newY = targetPosY;
    }

        bool canMove = true;
    float entityHalfSize = TILE_SIZE / 2.0f;

    // Check only the center and the direction of movement
    int checkPoints[2][2] = {{0, 0}, {(int)sgn(moveX), (int)sgn(moveY)}};
    for (int i = 0; i < 2; i++) {
        float checkX = newX + checkPoints[i][0] * entityHalfSize;
        float checkY = newY + checkPoints[i][1] * entityHalfSize;
        
        int gridX = (int)((checkX + 1.0f) * GRID_SIZE / 2);
        int gridY = (int)((1.0f - checkY) * GRID_SIZE / 2);

        if (!isWalkable(gridX, gridY)) {
            canMove = false;
            if (entity->isPlayer) {
                printf("Player collision at point (%d, %d) with tile (%d, %d)\n", 
                       checkPoints[i][0], checkPoints[i][1], gridX, gridY);
            }
            break;
        }
    }
    if (canMove) {
        for (int i = 0; i < entityCount; i++) {
            if (allEntities[i] != entity) {
                float otherX = allEntities[i]->posX;
                float otherY = allEntities[i]->posY;
                float distX = fabs(newX - otherX);
                float distY = fabs(newY - otherY);
                
                if (distX < TILE_SIZE && distY < TILE_SIZE) {
                    canMove = false;
                    if (entity->isPlayer) {
                        printf("Player collision with entity at (%f, %f)\n", otherX, otherY);
                    }
                    break;
                }
            }
        }
    }

    if (canMove) {
        entity->posX = newX;
        entity->posY = newY;
        
        int newGridX = (int)((newX + 1.0f) * GRID_SIZE / 2);
        int newGridY = (int)((1.0f - newY) * GRID_SIZE / 2);
        
        if (newGridX != entity->gridX || newGridY != entity->gridY) {
            if (isWalkable(newGridX, newGridY)) {
                entity->gridX = newGridX;
                entity->gridY = newGridY;
            } else {
                // If the new grid position is not walkable, find the nearest walkable tile
                findNearestWalkableTile(newX, newY, &entity->gridX, &entity->gridY);
                entity->needsPathfinding = true;
            }
        }
        
        if (entity->isPlayer) {
            printf("Player moved to: (%f, %f) Grid: (%d, %d)\n", entity->posX, entity->posY, entity->gridX, entity->gridY);
        }
    } else {
        // If can't move, find the nearest walkable tile
        findNearestWalkableTile(entity->posX, entity->posY, &entity->gridX, &entity->gridY);
        entity->posX = (2.0f * entity->gridX / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE);
        entity->posY = 1.0f - (2.0f * entity->gridY / GRID_SIZE) - (1.0f / GRID_SIZE);
        entity->needsPathfinding = true;
        
        if (entity->isPlayer) {
            printf("Player couldn't move, snapped to nearest walkable tile: (%d, %d)\n", entity->gridX, entity->gridY);
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