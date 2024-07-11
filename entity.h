// entity.h
#ifndef ENTITY_H
#define ENTITY_H

#include <stdbool.h>

typedef struct {
    float posX;
    float posY;
    int gridX;
    int gridY;
    float speed;
    int targetGridX;
    int targetGridY;
    bool needsPathfinding;
    // Add any other necessary fields
} Entity;

void UpdateEntity(Entity* entity, Entity** allEntities, int entityCount);

#endif // ENTITY_H