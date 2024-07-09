#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include "pathfinding.h"
#include "grid.h"
#include "entity.h"

PriorityQueue* createPriorityQueue(int capacity) {
    PriorityQueue* pq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
    pq->nodes = (Node*)malloc(sizeof(Node) * capacity);
    pq->size = 0;
    pq->capacity = capacity;
    return pq;
}

void swap(Node* a, Node* b) {
    Node temp = *a;
    *a = *b;
    *b = temp;
}

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

void push(PriorityQueue* pq, Node node) {
    if (pq->size == pq->capacity) {
        pq->capacity *= 2;
        pq->nodes = (Node*)realloc(pq->nodes, sizeof(Node) * pq->capacity);
    }
    pq->nodes[pq->size] = node;
    heapifyUp(pq, pq->size);
    pq->size++;
}

Node pop(PriorityQueue* pq) {
    Node top = pq->nodes[0];
    pq->nodes[0] = pq->nodes[pq->size - 1];
    pq->size--;
    heapifyDown(pq, 0);
    return top;
}

float heuristic(int x1, int y1, int x2, int y2) {
    return sqrtf((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

Node* findPath(int startX, int startY, int goalX, int goalY) {
    PriorityQueue* openList = createPriorityQueue(GRID_SIZE * GRID_SIZE);
    bool closedList[GRID_SIZE][GRID_SIZE] = {false};
    Node* nodes = (Node*)malloc(sizeof(Node) * GRID_SIZE * GRID_SIZE);

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            nodes[i * GRID_SIZE + j] = (Node){j, i, INFINITY, INFINITY, INFINITY, NULL};
        }
    }

    Node* startNode = &nodes[startY * GRID_SIZE + startX];
    startNode->g = 0;
    startNode->h = heuristic(startX, startY, goalX, goalY);
    startNode->f = startNode->g + startNode->h;

    push(openList, *startNode);

    int dx[] = {-1, 0, 1, 0, -1, -1, 1, 1};
    int dy[] = {0, -1, 0, 1, -1, 1, -1, 1};

    while (openList->size > 0) {
        Node current = pop(openList);

        if (current.x == goalX && current.y == goalY) {
            free(openList->nodes);
            free(openList);
            return nodes;
        }

        closedList[current.y][current.x] = true;

        for (int i = 0; i < 8; i++) {
            int newX = current.x + dx[i];
            int newY = current.y + dy[i];

            if (!isWalkable(newX, newY) || closedList[newY][newX]) {
                continue;
            }

            float newG = current.g + ((i < 4) ? 1.0f : 1.414f);
            Node* neighbor = &nodes[newY * GRID_SIZE + newX];

            if (newG < neighbor->g) {
                neighbor->parent = &nodes[current.y * GRID_SIZE + current.x];
                neighbor->g = newG;
                neighbor->h = heuristic(newX, newY, goalX, goalY);
                neighbor->f = neighbor->g + neighbor->h;

                if (neighbor->parent == NULL) {
                    push(openList, *neighbor);
                }
            }
        }
    }

    free(openList->nodes);
    free(openList);
    free(nodes);
    return NULL;
}

void updateEntityPath(Entity* entity) {
    int startX = (int)((entity->posX + 1.0f) * GRID_SIZE / 2);
    int startY = (int)((1.0f - entity->posY) * GRID_SIZE / 2);
    int goalX = (int)((entity->targetPosX + 1.0f) * GRID_SIZE / 2);
    int goalY = (int)((1.0f - entity->targetPosY) * GRID_SIZE / 2);

    Node* path = findPath(startX, startY, goalX, goalY);

    if (path) {
        Node* current = &path[goalY * GRID_SIZE + goalX];
        Node* next = current;
        
        while (next->parent && (next->parent->x != startX || next->parent->y != startY)) {
            next = next->parent;
        }

        if (next && next != current) {
            entity->targetPosX = (2.0f * next->x / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE);
            entity->targetPosY = 1.0f - (2.0f * next->y / GRID_SIZE) - (1.0f / GRID_SIZE);
        }

        free(path);
    } else {
        // If no path is found, don't change the target
        printf("No path found from (%d, %d) to (%d, %d)\n", startX, startY, goalX, goalY);
    }
}