#include "entity.h"
#include "grid.h"
#include "gameloop.h"
#include "pathfinding.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

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

    // If very close to target, snap to it (but only if the target is walkable)
    if (distance < 0.001f && isWalkable(entity->targetGridX, entity->targetGridY)) {
        entity->posX = targetPosX;
        entity->posY = targetPosY;
        entity->gridX = entity->targetGridX;
        entity->gridY = entity->targetGridY;
        entity->needsPathfinding = true;
        if (entity->isPlayer) {
            printf("Player snapped to target (%d, %d)\n", entity->gridX, entity->gridY);
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

    int cornerChecks[4][2] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
    for (int i = 0; i < 4; i++) {
        float cornerX = newX + cornerChecks[i][0] * entityHalfSize;
        float cornerY = newY + cornerChecks[i][1] * entityHalfSize;
        
        int gridX = (int)((cornerX + 1.0f) * GRID_SIZE / 2);
        int gridY = (int)((1.0f - cornerY) * GRID_SIZE / 2);

        if (!isWalkable(gridX, gridY)) {
            canMove = false;
            if (entity->isPlayer) {
                printf("Player collision at corner (%d, %d) with tile (%d, %d)\n", 
                       cornerChecks[i][0], cornerChecks[i][1], gridX, gridY);
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
        
        // Update grid position if we've reached the target
        if (newX == targetPosX && newY == targetPosY && isWalkable(entity->targetGridX, entity->targetGridY)) {
            entity->gridX = entity->targetGridX;
            entity->gridY = entity->targetGridY;
            entity->needsPathfinding = true;
        } else {
            // Otherwise, update grid position based on current position
            int newGridX = (int)((newX + 1.0f) * GRID_SIZE / 2);
            int newGridY = (int)((1.0f - newY) * GRID_SIZE / 2);
            
            if (newGridX != entity->gridX || newGridY != entity->gridY) {
                if (isWalkable(newGridX, newGridY)) {
                    entity->gridX = newGridX;
                    entity->gridY = newGridY;
                } else {
                    // If the new grid position is not walkable, don't update it
                    entity->needsPathfinding = true;
                }
            }
        }
        
        if (entity->isPlayer) {
            printf("Player moved to: (%f, %f) Grid: (%d, %d)\n", entity->posX, entity->posY, entity->gridX, entity->gridY);
        }
    } else {
        if (entity->isPlayer) {
            printf("Player couldn't move, stuck at: (%f, %f) Grid: (%d, %d)\n", entity->posX, entity->posY, entity->gridX, entity->gridY);
        }
        entity->needsPathfinding = true;
    }
}
void updateEntityPath(Entity* entity) {
    int startX = entity->gridX;
    int startY = entity->gridY;
    int goalX = entity->finalGoalX;
    int goalY = entity->finalGoalY;

    if (startX == goalX && startY == goalY) {
        if (entity->isPlayer) {
            printf("Player already at final goal (%d, %d)\n", startX, startY);
        }
        entity->targetGridX = startX;
        entity->targetGridY = startY;
        entity->needsPathfinding = false;
        return;
    }

    if (!isWalkable(startX, startY) || !isWalkable(goalX, goalY)) {
        if (entity->isPlayer) {
            printf("Player: Start or goal is not walkable. Start: (%d, %d), Goal: (%d, %d)\n", startX, startY, goalX, goalY);
        }
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
        entity->currentPathIndex = 0;

        // Count path length and print path
        Node* current = &path[goalY * GRID_SIZE + goalX];
        printf("Path: ");
        while (current) {
            entity->cachedPathLength++;
            printf("(%d, %d) ", current->x, current->y);
            current = current->parent;
        }
        printf("\n");

        if (entity->isPlayer) {
            printf("Player: Path found. Length: %d\n", entity->cachedPathLength);
        }

        // Find the next step in the path
        Node* next = &path[goalY * GRID_SIZE + goalX];
        while (next->parent && next->parent->parent) {
            next = next->parent;
        }

        if (next && (next->x != startX || next->y != startY)) {
            entity->targetGridX = next->x;
            entity->targetGridY = next->y;
            
            if (entity->isPlayer) {
                printf("Player: Next step in path: (%d, %d)\n", entity->targetGridX, entity->targetGridY);
            }
        } else {
            entity->targetGridX = startX;
            entity->targetGridY = startY;
            if (entity->isPlayer) {
                printf("Player: No valid next step found. Player at (%d, %d)\n", entity->gridX, entity->gridY);
            }
        }
    } else {
        if (entity->isPlayer) {
            printf("Player: No path found from (%d, %d) to (%d, %d)\n", startX, startY, goalX, goalY);
        }
        entity->targetGridX = startX;
        entity->targetGridY = startY;
    }

    entity->needsPathfinding = false;
}

