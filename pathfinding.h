#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "grid.h"
#include <stdbool.h>
#include <GL/glew.h>

typedef struct Node {
    int x, y;
    float g, h, f;
    struct Node* parent;
} Node;

typedef struct {
    int x, y;
    float g, h, f;
    int parentX, parentY;
} GPUNode;

typedef struct {
    Node* nodes;
    int size;
    int capacity;
} PriorityQueue;



// CPU-based A* functions
void destroyPriorityQueue(PriorityQueue* pq);
PriorityQueue* createPriorityQueue(int capacity);
bool inPriorityQueue(PriorityQueue* pq, Node node);
void swap(Node* a, Node* b);
void heapifyUp(PriorityQueue* pq, int index);
void heapifyDown(PriorityQueue* pq, int index);
void push(PriorityQueue* pq, Node node);
Node pop(PriorityQueue* pq);

float heuristic(int x1, int y1, int x2, int y2);
bool lineOfSight(int x0, int y0, int x1, int y1);
Node* findPath(int startX, int startY, int goalX, int goalY, int* pathLength);

// GPU-based A* functions
void initializeGPUPathfinding();
Node* findPathGPU(int startX, int startY, int goalX, int goalY, int* pathLength);
void cleanupGPUPathfinding();

#endif // PATHFINDING_H