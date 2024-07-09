// entity.c
#include "entity.h"
#include "pathfinding.h"
#include "gameloop.h"
#include "grid.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
void InitEntity(Entity* entity, float startX, float startY, float speed) {
    entity->posX = startX;
    entity->posY = startY;
    entity->targetPosX = startX;
    entity->targetPosY = startY;
    entity->moveSpeed = speed;
    entity->needsPathfinding = false;
}

bool IsObstructed(float startX, float startY, float endX, float endY) {
    int startGridX = (int)((startX + 1.0f) * GRID_SIZE / 2);
    int startGridY = (int)((1.0f - startY) * GRID_SIZE / 2);
    int endGridX = (int)((endX + 1.0f) * GRID_SIZE / 2);
    int endGridY = (int)((1.0f - endY) * GRID_SIZE / 2);

    // Simple line drawing algorithm to check for obstructions
    int dx = abs(endGridX - startGridX);
    int dy = abs(endGridY - startGridY);
    int sx = startGridX < endGridX ? 1 : -1;
    int sy = startGridY < endGridY ? 1 : -1;
    int err = dx - dy;

    while (startGridX != endGridX || startGridY != endGridY) {
        if (startGridX < 0 || startGridX >= GRID_SIZE || startGridY < 0 || startGridY >= GRID_SIZE ||
            !grid[startGridY][startGridX].walkable) {
            return true; // Obstruction found
        }

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            startGridX += sx;
        }
        if (e2 < dx) {
            err += dx;
            startGridY += sy;
        }
    }

    return false; // No obstruction
}

void UpdateEntity(Entity* entity) {
    float dx = entity->targetPosX - entity->posX;
    float dy = entity->targetPosY - entity->posY;
    float distance = sqrt(dx * dx + dy * dy);

    if (distance > entity->moveSpeed) {
        float normalizedDx = dx / distance;
        float normalizedDy = dy / distance;
        
        float nextX = entity->posX + normalizedDx * entity->moveSpeed;
        float nextY = entity->posY + normalizedDy * entity->moveSpeed;
        
        if (!IsObstructed(entity->posX, entity->posY, nextX, nextY)) {
            entity->posX = nextX;
            entity->posY = nextY;
        } else {
            entity->needsPathfinding = true;
        }
    } else {
        entity->posX = entity->targetPosX;
        entity->posY = entity->targetPosY;
        entity->needsPathfinding = true;  // Request new path when target is reached
    }

    if (entity->needsPathfinding) {
        updateEntityPath(entity);
        entity->needsPathfinding = false;
    }
}