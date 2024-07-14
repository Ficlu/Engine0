
#include "entity.h"
#include "grid.h"
#include "gameloop.h"
#include "pathfinding.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
/*Converts the target-coordinate properties for the input entity to tile-coords, calulates 
the current distance to the target-tile, moves the entity to the target if the entity can 
reach the target during it's current step--otherwise moving in the direction of the target.
- Uses grid.isWalkable to check grid-cells
- Uses entity.lineOfSight to see if path-finding is required
- Uses entity.updateEntityPath to run the path-finding search algorithm
- Used to update the position of an entity such that it moves from it's current position
to the target-tile, triggering path-finding if necessary */
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

/*Attempts to find a path from the entities current position to it's target position by retrieving
the current and target positions from the entity, checking their walkability, finding a path between them
and moves the entity but updating it's coordinates to progress on a path, if such a path is available
- Uses grid.isWalkable to check walkability of tiles
- Uses pathFinding.findPath to run the path-finding search algorithm if necessary
- Used to path-find to the target-tile when non-linear traversal is required*/
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
        // Update the entity's current path
        if (entity->currentPath) {
            free(entity->currentPath);
        }
        entity->currentPath = path;
        entity->pathLength = 0;

        Node* current = &path[goalY * GRID_SIZE + goalX];
        while (current) {
            entity->pathLength++;
            current = current->parent;
        }

        Node* next = &path[goalY * GRID_SIZE + goalX];
        while (next->parent && (next->parent->x != startX || next->parent->y != startY)) {
            next = next->parent;
        }

        if (next && next != &path[goalY * GRID_SIZE + goalX]) {
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
    } else {
        printf("No path found from (%d, %d) to (%d, %d)\n", startX, startY, goalX, goalY);
    }
}