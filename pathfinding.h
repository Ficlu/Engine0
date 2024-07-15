#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "grid.h"
#include <stdbool.h>

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
bool inPriorityQueue(PriorityQueue* pq, Node node);
void swap(Node* a, Node* b);
void heapifyUp(PriorityQueue* pq, int index);
void heapifyDown(PriorityQueue* pq, int index);
void push(PriorityQueue* pq, Node node);
Node pop(PriorityQueue* pq);

float heuristic(int x1, int y1, int x2, int y2);
bool lineOfSight(int x0, int y0, int x1, int y1);
Node* findPath(int startX, int startY, int goalX, int goalY);

#endif // PATHFINDING_H