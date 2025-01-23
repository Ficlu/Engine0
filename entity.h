#ifndef ENTITY_H
#define ENTITY_H

#include "grid.h"
#include <stdbool.h>
#include <stdatomic.h>

// Forward declaration
struct Node;

typedef struct {
    atomic_int gridX;
    atomic_int gridY;
    _Atomic float posX;
    _Atomic float posY;
    float speed;
    atomic_int targetGridX;
    atomic_int targetGridY;
    atomic_int finalGoalX;
    atomic_int finalGoalY;
    atomic_bool needsPathfinding;
    struct Node* cachedPath;
    int cachedPathLength;
    int currentPathIndex;
    bool isPlayer;
    
} Entity;

void findNearestWalkableTile(float posX, float posY, int* nearestX, int* nearestY);
void UpdateEntity(Entity* entity, Entity** allEntities, int entityCount);
void updateEntityPath(Entity* entity);

#endif // ENTITY_H