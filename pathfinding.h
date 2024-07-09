#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "entity.h"
#include "grid.h"

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
Node* findPath(int startX, int startY, int goalX, int goalY);
void updateEntityPath(Entity* entity);

#endif // PATHFINDING_H