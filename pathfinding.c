// pathfinding.c

#include "pathfinding.h"
#include "entity.h"
#include "grid.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <float.h>

/*
 * createPriorityQueue
 *
 * Creates a priority queue with the given capacity.
 *
 * @param[in] capacity The capacity of the priority queue
 * @return PriorityQueue* Pointer to the created priority queue
 */
PriorityQueue* createPriorityQueue(int capacity) {
    PriorityQueue* pq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
    pq->nodes = (Node*)malloc(sizeof(Node) * capacity);
    pq->size = 0;
    pq->capacity = capacity;
    return pq;
}

/*
 * inPriorityQueue
 *
 * Checks if a node is in the priority queue.
 *
 * @param[in] pq Pointer to the priority queue
 * @param[in] node The node to check
 * @return bool True if the node is in the priority queue, false otherwise
 */
bool inPriorityQueue(PriorityQueue* pq, Node node) {
    for (int i = 0; i < pq->size; i++) {
        if (pq->nodes[i].x == node.x && pq->nodes[i].y == node.y) {
            return true;
        }
    }
    return false;
}

/*
 * swap
 *
 * Swaps two nodes.
 *
 * @param[in,out] a Pointer to the first node
 * @param[in,out] b Pointer to the second node
 */
void swap(Node* a, Node* b) {
    Node temp = *a;
    *a = *b;
    *b = temp;
}

/*
 * heapifyUp
 *
 * Maintains the heap property by moving a node up the heap.
 *
 * @param[in] pq Pointer to the priority queue
 * @param[in] index The index of the node to move up
 */
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

/*
 * heapifyDown
 *
 * Maintains the heap property by moving a node down the heap.
 *
 * @param[in] pq Pointer to the priority queue
 * @param[in] index The index of the node to move down
 */
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

/*
 * push
 *
 * Adds a node to the priority queue.
 *
 * @param[in,out] pq Pointer to the priority queue
 * @param[in] node The node to add
 */
void push(PriorityQueue* pq, Node node) {
    if (pq->size == pq->capacity) {
        pq->capacity *= 2;
        pq->nodes = (Node*)realloc(pq->nodes, sizeof(Node) * pq->capacity);
    }
    pq->nodes[pq->size] = node;
    heapifyUp(pq, pq->size);
    pq->size++;
}

/*
 * pop
 *
 * Removes and returns the node with the highest priority (lowest f value).
 *
 * @param[in,out] pq Pointer to the priority queue
 * @return Node The node with the highest priority
 */
Node pop(PriorityQueue* pq) {
    Node top = pq->nodes[0];
    pq->nodes[0] = pq->nodes[pq->size - 1];
    pq->size--;
    heapifyDown(pq, 0);
    return top;
}

/*
 * heuristic
 *
 * Calculates the heuristic distance between two points.
 *
 * @param[in] x1 The x-coordinate of the first point
 * @param[in] y1 The y-coordinate of the first point
 * @param[in] x2 The x-coordinate of the second point
 * @param[in] y2 The y-coordinate of the second point
 * @return float The heuristic distance between the points
 */
float heuristic(int x1, int y1, int x2, int y2) {
    return sqrtf((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

/*
 * lineOfSight
 *
 * Checks if there is a clear line of sight between two points.
 *
 * @param[in] x0 The x-coordinate of the starting point
 * @param[in] y0 The y-coordinate of the starting point
 * @param[in] x1 The x-coordinate of the ending point
 * @param[in] y1 The y-coordinate of the ending point
 * @return bool True if there is a clear line of sight, false otherwise
 */
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
            if (!isWalkable(x + x_inc, y) || !isWalkable(x, y + y_inc)) return false;
            x += x_inc;
            y += y_inc;
            error += dx - dy;
            n--;
        }
    }
    return true;
}

/*
 * findPath
 *
 * Finds a path from the start position to the goal position using the A* algorithm.
 *
 * @param[in] startX The x-coordinate of the start position
 * @param[in] startY The y-coordinate of the start position
 * @param[in] goalX The x-coordinate of the goal position
 * @param[in] goalY The y-coordinate of the goal position
 * @return Node* Pointer to the array of nodes representing the path, or NULL if no path is found
 */
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

            float newG = current.g + 1.0f;

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
    return NULL;
}
