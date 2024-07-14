
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

    // Calculate the target position in world coordinates
    float targetPosX = (2.0f * entity->targetGridX / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE);
    float targetPosY = 1.0f - (2.0f * entity->targetGridY / GRID_SIZE) - (1.0f / GRID_SIZE);

    // Calculate the distance to the target
    float dx = targetPosX - entity->posX;
    float dy = targetPosY - entity->posY;
    float distance = sqrt(dx*dx + dy*dy);

    // If we're close enough to the target, snap to it
    if (distance < 0.001f) {
        entity->posX = targetPosX;
        entity->posY = targetPosY;
        entity->gridX = entity->targetGridX;
        entity->gridY = entity->targetGridY;

        if (entity->isPlayer) {
            printf("Player reached target (%d, %d)\n", entity->gridX, entity->gridY);
        }

        // Request new pathfinding if we've reached our target
        entity->needsPathfinding = true;
        return;
    }

    // Calculate movement for this frame
    float moveDistance = fmin(entity->speed, distance);
    float moveX = (dx / distance) * moveDistance;
    float moveY = (dy / distance) * moveDistance;

    // Calculate new position
    float newX = entity->posX + moveX;
    float newY = entity->posY + moveY;

    // Convert new position to grid coordinates
    int newGridX = (int)((newX + 1.0f) * GRID_SIZE / 2);
    int newGridY = (int)((1.0f - newY) * GRID_SIZE / 2);

    // Check if the new position is walkable
    bool canMove = isWalkable(newGridX, newGridY);

    // Check for collisions with other entities
    if (canMove) {
        for (int i = 0; i < entityCount; i++) {
            if (allEntities[i] != entity) {
                if (newGridX == allEntities[i]->gridX && newGridY == allEntities[i]->gridY) {
                    canMove = false;
                    break;
                }
            }
        }
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
        // If the entity can't move, request a new path
        entity->needsPathfinding = true;
        if (entity->isPlayer) {
            printf("Player couldn't move, requesting new path\n");
        }
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
        if (entity->isPlayer) {
            printf("Player: Start or goal is not walkable. Start: (%d, %d), Goal: (%d, %d)\n", startX, startY, goalX, goalY);
        }
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

        if (entity->isPlayer) {
            printf("Player: Path found. Length: %d\n", entity->pathLength);
        }

        // Set the target to the final goal
        entity->targetGridX = goalX;
        entity->targetGridY = goalY;

        if (entity->isPlayer) {
            printf("Player: Moving to final goal: (%d, %d)\n", entity->targetGridX, entity->targetGridY);
        }
    } else {
        if (entity->isPlayer) {
            printf("Player: No path found from (%d, %d) to (%d, %d)\n", startX, startY, goalX, goalY);
        }
        // If no path is found, set the target to the current position
        entity->targetGridX = startX;
        entity->targetGridY = startY;
    }
}