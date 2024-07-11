// entity.c
#include "entity.h"
#include "grid.h"
#include "gameloop.h"
#include "pathfinding.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
void UpdateEntity(Entity* entity, Entity** allEntities, int entityCount) {
    if (entity->needsPathfinding) {
        updateEntityPath(entity);
        entity->needsPathfinding = false;
    }

    // Move towards target
    float dx = (2.0f * entity->targetGridX / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE) - entity->posX;
    float dy = 1.0f - (2.0f * entity->targetGridY / GRID_SIZE) - (1.0f / GRID_SIZE) - entity->posY;

    float distance = sqrt(dx*dx + dy*dy);
    if (distance > entity->speed) {
        dx = (dx / distance) * entity->speed;
        dy = (dy / distance) * entity->speed;
    }

    // Check for collisions with other entities and unwalkable tiles
    bool canMove = true;
    float newX = entity->posX + dx;
    float newY = entity->posY + dy;
    int newGridX = (int)((newX + 1.0f) * GRID_SIZE / 2);
    int newGridY = (int)((1.0f - newY) * GRID_SIZE / 2);

    // Check if the new position is walkable
    if (!isWalkable(newGridX, newGridY)) {
        canMove = false;
    }

    // Check for collisions with other entities
    for (int i = 0; i < entityCount; i++) {
        if (allEntities[i] != entity) {
            float otherX = allEntities[i]->posX;
            float otherY = allEntities[i]->posY;
            
            if (fabs(newX - otherX) < TILE_SIZE && fabs(newY - otherY) < TILE_SIZE) {
                canMove = false;
                break;
            }
        }
    }

    if (canMove) {
        entity->posX = newX;
        entity->posY = newY;
        entity->gridX = newGridX;
        entity->gridY = newGridY;
    } else {
        // If the entity can't move, request a new path
        entity->needsPathfinding = true;
    }
}

void updateEntityPath(Entity* entity) {
    int startX = entity->gridX;
    int startY = entity->gridY;
    int goalX = entity->targetGridX;
    int goalY = entity->targetGridY;

    // Ensure start and goal are within bounds and walkable
    if (!isWalkable(startX, startY) || !isWalkable(goalX, goalY)) {
        printf("Start or goal is not walkable. Start: (%d, %d), Goal: (%d, %d)\n", startX, startY, goalX, goalY);
        return;
    }

    Node* path = findPath(startX, startY, goalX, goalY);

    if (path) {
        Node* current = &path[goalY * GRID_SIZE + goalX];
        Node* next = current;
        
        while (next->parent && (next->parent->x != startX || next->parent->y != startY)) {
            next = next->parent;
        }

        if (next && next != current) {
            // Add a small random offset to prevent getting stuck on grid alignments
            int offsetX = (rand() % 3) - 1;  // -1, 0, or 1
            int offsetY = (rand() % 3) - 1;  // -1, 0, or 1
            
            entity->targetGridX = next->x + offsetX;
            entity->targetGridY = next->y + offsetY;

            // Ensure the new target is within grid bounds
            entity->targetGridX = (entity->targetGridX < 0) ? 0 : (entity->targetGridX >= GRID_SIZE) ? GRID_SIZE - 1 : entity->targetGridX;
            entity->targetGridY = (entity->targetGridY < 0) ? 0 : (entity->targetGridY >= GRID_SIZE) ? GRID_SIZE - 1 : entity->targetGridY;
        } else {
            // If no valid next step, don't change the target
            printf("No valid next step found. Entity at (%d, %d)\n", entity->gridX, entity->gridY);
        }

        free(path);
    } else {
        printf("No path found from (%d, %d) to (%d, %d)\n", startX, startY, goalX, goalY);
    }
}