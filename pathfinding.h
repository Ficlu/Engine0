// pathfinding.h
#ifndef PATHFINDING_H
#define PATHFINDING_H

#include <stdbool.h>
#include "entity.h"

#define GRID_SIZE 20  // Make sure this matches the definition in other files

typedef struct Node {
    int x, y;
    float g, h, f;
    struct Node* parent;
} Node;

typedef struct {
    Node* nodes;
    int size;
    int capacity;
} PriorityQueue;

PriorityQueue* createPriorityQueue(int capacity);
void push(PriorityQueue* pq, Node node);
Node pop(PriorityQueue* pq);
float heuristic(int x1, int y1, int x2, int y2);
bool lineOfSight(int x0, int y0, int x1, int y1);
Node* findPath(int startX, int startY, int goalX, int goalY);
void updateEntityPath(Entity* entity);
void unstuckEntity(Entity* entity);
void avoidCollision(Entity* entity, Entity** otherEntities, int entityCount);

#endif // PATHFINDING_H