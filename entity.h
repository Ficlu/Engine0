// entity.h
#ifndef ENTITY_H
#define ENTITY_H

#include "grid.h"
#include <stdbool.h>

// Forward declaration
struct Node;

typedef struct {
    int gridX;
    int gridY;
    float posX;
    float posY;
    float speed;
    int targetGridX;
    int targetGridY;
    int finalGoalX;
    int finalGoalY;
    bool needsPathfinding;
    struct Node* cachedPath;  // Use the forward-declared struct
    int cachedPathLength;
    int currentPathIndex;
    bool isPlayer;
} Entity;

void UpdateEntity(Entity* entity, Entity** allEntities, int entityCount);
void updateEntityPath(Entity* entity);

#endif // ENTITY_H