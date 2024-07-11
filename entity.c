// entity.c
#include "entity.h"
#include "pathfinding.h"
#include "gameloop.h"
#include <math.h>

void UpdateEntity(Entity* entity, Entity** allEntities, int entityCount) {
    if (entity->needsPathfinding) {
        // Perform pathfinding here
        // This is a placeholder - replace with actual pathfinding logic
        entity->needsPathfinding = false;
    }

    // Move towards target
    float dx = ((2.0f * entity->targetGridX / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE)) - entity->posX;
    float dy = (1.0f - (2.0f * entity->targetGridY / GRID_SIZE) - (1.0f / GRID_SIZE)) - entity->posY;

    float distance = sqrt(dx*dx + dy*dy);
    if (distance > entity->speed) {
        dx = (dx / distance) * entity->speed;
        dy = (dy / distance) * entity->speed;
    }

    // Check for collisions with other entities
    bool canMove = true;
    for (int i = 0; i < entityCount; i++) {
        if (allEntities[i] != entity) {
            float newX = entity->posX + dx;
            float newY = entity->posY + dy;
            float otherX = allEntities[i]->posX;
            float otherY = allEntities[i]->posY;
            
            if (fabs(newX - otherX) < TILE_SIZE && fabs(newY - otherY) < TILE_SIZE) {
                canMove = false;
                break;
            }
        }
    }

    if (canMove) {
        entity->posX += dx;
        entity->posY += dy;
    }

    // Update grid position
    entity->gridX = (int)((entity->posX + 1.0f) * GRID_SIZE / 2);
    entity->gridY = (int)((1.0f - entity->posY) * GRID_SIZE / 2);
}