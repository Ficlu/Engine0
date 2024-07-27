// pathfinding.c

#include "pathfinding.h"
#include "entity.h"
#include "grid.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#include <float.h>
/*Initialize and allocate memory to priorityQueue*/
PriorityQueue* createPriorityQueue(int capacity) {
    PriorityQueue* pq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
    pq->nodes = (Node*)malloc(sizeof(Node) * capacity);
    pq->size = 0;
    pq->capacity = capacity;
    return pq;
}

/*Checks if an input PriorityQueue has, as a member, the input node*/
bool inPriorityQueue(PriorityQueue* pq, Node node) {
    for (int i = 0; i < pq->size; i++) {
        if (pq->nodes[i].x == node.x && pq->nodes[i].y == node.y) {
            return true;
        }
    }
    return false;
}

/*Swaps the values of two nodes*/
void swap(Node* a, Node* b) {
    Node temp = *a;
    *a = *b;
    *b = temp;
}

/*For the input PriorityQueue, this function takes the value of the heuristic (f-value) at the 
input index, compares it to the f-value of the parent-node and swaps the parent and child; repeating
the process until the pq satisfies the heap property.
- Nodes in the min-heap are ordered by their heuristic value for the pathfinding algorithm
- Maintains heap property--our desired ordering for the PriorityQueue
- Used when inserting new nodes into a pq*/
void heapifyUp(PriorityQueue* pq, int index) {
    while (index > 0) {
        int parent = (index - 1) / 2;
        if (pq->nodes[index].f < pq->nodes[parent].f) {
            swap(&pq->nodes[index], &pq->nodes[parent]);
            index = parent;
        } else {
            break;
        }
    }
}

/*For the input PirorityQueue, this function takes the value of the heuristic (f-value) at the 
input index, compares it to the f-value of the child-node(s) and if either child's f-value is 
smaller than the parent node's, swaps the parent with the smallest child; repeating the process 
until the pq satisfies the heap property.
- Nodes in the min-heap are ordered by their heuristic value for the pathfinding algorithm
- Maintains heap property--our desired ordering for the PriorityQueue
- Used when removing the root node from a pq*/
void heapifyDown(PriorityQueue* pq, int index) {
    int smallest = index;
    int left = 2 * index + 1;
    int right = 2 * index + 2;

    if (left < pq->size && pq->nodes[left].f < pq->nodes[smallest].f)
        smallest = left;

    if (right < pq->size && pq->nodes[right].f < pq->nodes[smallest].f)
        smallest = right;

    if (smallest != index) {
        swap(&pq->nodes[index], &pq->nodes[smallest]);
        heapifyDown(pq, smallest);
    }
}

/*Adds a new node to the priorityQueue, resizes the pq if it exceeds capacity. 
- Calls heapifyUp function to maintain the heap property of pq*/
void push(PriorityQueue* pq, Node node) {
    if (pq->size == pq->capacity) {
        pq->capacity *= 2;
        pq->nodes = (Node*)realloc(pq->nodes, sizeof(Node) * pq->capacity);
    }
    pq->nodes[pq->size] = node;
    heapifyUp(pq, pq->size);
    pq->size++;
}

/*Removes and returns the node with the smallest f-value from the priorityQueue.
- Calls heapifyDown function to maintain heap property of pq*/
Node pop(PriorityQueue* pq) {
    Node top = pq->nodes[0];
    pq->nodes[0] = pq->nodes[pq->size - 1];
    pq->size--;
    heapifyDown(pq, 0);
    return top;
}

/*Calculates the euclidean distance between the two tiles at the input coords
- Used in determining which candidate tiles to explore in the search algorithm.*/
float heuristic(int x1, int y1, int x2, int y2) {
    return sqrtf((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

/*Checks if there is direct line-of-sight between the two tiles at the input coords
- Used to detect unwalkable tiles which determine whether or not the entity needs to use 
a search algorithm to reach it's target-tile*/
bool lineOfSight(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int x = x0;
    int y = y0;
    int n = 1 + dx + dy;
    int x_inc = (x1 > x0) ? 1 : -1;
    int y_inc = (y1 > y0) ? 1 : -1;
    int error = dx - dy;
    dx *= 2;
    dy *= 2;

    for (; n > 0; --n) {
        if (!isWalkable(x, y)) return false;
        
        if (error > 0) {
            x += x_inc;
            error -= dy;
        } else if (error < 0) {
            y += y_inc;
            error += dx;
        } else {
            // Check both diagonal neighbors
            if (!isWalkable(x + x_inc, y) || !isWalkable(x, y + y_inc)) return false;
            x += x_inc;
            y += y_inc;
            error += dx - dy;
            n--;
        }
    }
    return true;
}

/*Implements a search algorithm to pathfind from the tile at the start coords 
to the tile at the goal coords */
Node* findPath(int startX, int startY, int goalX, int goalY) {
    PriorityQueue* openList = createPriorityQueue(GRID_SIZE * GRID_SIZE);
    bool** closedList = (bool**)malloc(sizeof(bool*) * GRID_SIZE);
    for (int i = 0; i < GRID_SIZE; i++) {
        closedList[i] = (bool*)calloc(GRID_SIZE, sizeof(bool));
    }
    Node* nodes = (Node*)malloc(sizeof(Node) * GRID_SIZE * GRID_SIZE);

    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            nodes[y * GRID_SIZE + x] = (Node){x, y, INFINITY, INFINITY, INFINITY, NULL};
        }
    }

    Node* startNode = &nodes[startY * GRID_SIZE + startX];
    startNode->g = 0;
    startNode->h = heuristic(startX, startY, goalX, goalY);
    startNode->f = startNode->g + startNode->h;

    push(openList, *startNode);

    // Only consider orthogonal movements (no diagonals)
    int dx[] = {-1, 0, 1, 0};
    int dy[] = {0, -1, 0, 1};

    while (openList->size > 0) {
        Node current = pop(openList);

        if (current.x == goalX && current.y == goalY) {
            for (int i = 0; i < GRID_SIZE; i++) {
                free(closedList[i]);
            }
            free(closedList);
            free(openList->nodes);
            free(openList);
            return nodes;
        }

        closedList[current.y][current.x] = true;

        for (int i = 0; i < 4; i++) {
            int newX = current.x + dx[i];
            int newY = current.y + dy[i];

            if (!isValid(newX, newY) || !isWalkable(newX, newY) || closedList[newY][newX]) {
                continue;
            }

            float newG = current.g + 1.0f;  // Cost is always 1 for orthogonal movements

            Node* neighbor = &nodes[newY * GRID_SIZE + newX];
            if (newG < neighbor->g) {
                neighbor->parent = &nodes[current.y * GRID_SIZE + current.x];
                neighbor->g = newG;
                neighbor->h = heuristic(newX, newY, goalX, goalY);
                neighbor->f = neighbor->g + neighbor->h;

                if (!inPriorityQueue(openList, *neighbor)) {
                    push(openList, *neighbor);
                }
            }
        }
    }

    for (int i = 0; i < GRID_SIZE; i++) {
        free(closedList[i]);
    }
    free(closedList);
    free(openList->nodes);
    free(openList);
    return NULL;  // No path found
}