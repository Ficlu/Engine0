#ifndef ENTITY_H
#define ENTITY_H

#include <stdbool.h>

// Forward declaration of Node
struct Node;

typedef struct {
    float posX;
    float posY;
    int gridX;
    int gridY;
    float speed;
    int targetGridX;
    int targetGridY;
    bool needsPathfinding;
    // New fields for path rendering
    struct Node* currentPath;
    int pathLength;
} Entity;


void UpdateEntity(Entity* entity, Entity** allEntities, int entityCount);
void updateEntityPath(Entity* entity);
void InitEntity(Entity* entity, int startGridX, int startGridY, float speed);

#endif // ENTITY_H