// pathfinding.c

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

            Node* neighbor = &nodes[newY * GRID_SIZE + newX];

            if (current.parent == NULL || !lineOfSight(current.parent->x, current.parent->y, newX, newY)) {
                // No line of sight, use the current node as parent
                float newG = current.g + ((i < 4) ? 1.0f : 1.414f);
                if (newG < neighbor->g) {
                    neighbor->parent = &nodes[current.y * GRID_SIZE + current.x];
                    neighbor->g = newG;
                    neighbor->h = heuristic(newX, newY, goalX, goalY);
                    neighbor->f = neighbor->g + neighbor->h;

                    if (!closedList[newY][newX]) {
                        push(openList, *neighbor);
                    }
                }
            } else {
                // Line of sight exists, try to use grandparent
                float newG = current.parent->g + heuristic(current.parent->x, current.parent->y, newX, newY);
                if (newG < neighbor->g) {
                    neighbor->parent = current.parent;
                    neighbor->g = newG;
                    neighbor->h = heuristic(newX, newY, goalX, goalY);
                    neighbor->f = neighbor->g + neighbor->h;

                    if (!closedList[newY][newX]) {
                        push(openList, *neighbor);
                    }
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

    // Ensure start and goal are within bounds and walkable
    if (!isWalkable(startX, startY) || !isWalkable(goalX, goalY)) {
        printf("Start or goal is not walkable. Start: (%d, %d), Goal: (%d, %d)\n", startX, startY, goalX, goalY);
        return;
    }

    Node* path = findPath(startX, startY, goalX, goalY);

    if (path) {
        Node* current = &path[goalY * GRID_SIZE + goalX];
        Node* next = current;
        
        while (next->parent && (next->parent->x != startX || next->parent->y != startY)) {
            next = next->parent;
        }

        if (next && next != current) {
            // Add a small random offset to prevent getting stuck on grid alignments
            float offsetX = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
            float offsetY = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
            
            entity->targetPosX = (2.0f * next->x / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE) + offsetX;
            entity->targetPosY = 1.0f - (2.0f * next->y / GRID_SIZE) - (1.0f / GRID_SIZE) + offsetY;
        } else {
            // If no valid next step, don't change the target
            printf("No valid next step found. Entity at (%f, %f)\n", entity->posX, entity->posY);
        }

        free(path);
    } else {
        printf("No path found from (%d, %d) to (%d, %d)\n", startX, startY, goalX, goalY);
    }
}

void unstuckEntity(Entity* entity) {
    // Try to move the entity in a random direction if it's stuck
    float angle = ((float)rand() / RAND_MAX) * 2 * M_PI;
    float distance = 0.1f;  // Adjust this value based on your game's scale
    
    float newX = entity->posX + cosf(angle) * distance;
    float newY = entity->posY + sinf(angle) * distance;
    
    int gridX = (int)((newX + 1.0f) * GRID_SIZE / 2);
    int gridY = (int)((1.0f - newY) * GRID_SIZE / 2);
    
    if (isWalkable(gridX, gridY)) {
        entity->posX = newX;
        entity->posY = newY;
    }
}

void avoidCollision(Entity* entity, Entity** otherEntities, int entityCount) {
    const float COLLISION_RADIUS = 0.05f;  // Adjust based on your game's scale
    float totalForceX = 0, totalForceY = 0;

    for (int i = 0; i < entityCount; i++) {
        if (otherEntities[i] == entity) continue;

        float dx = entity->posX - otherEntities[i]->posX;
        float dy = entity->posY - otherEntities[i]->posY;
        float distSq = dx * dx + dy * dy;

        if (distSq < COLLISION_RADIUS * COLLISION_RADIUS && distSq > 0) {
            float dist = sqrtf(distSq);
            float force = (COLLISION_RADIUS - dist) / COLLISION_RADIUS;
            totalForceX += (dx / dist) * force;
            totalForceY += (dy / dist) * force;
        }
    }

    entity->posX += totalForceX * 0.01f;  // Adjust multiplier for stronger/weaker avoidance
    entity->posY += totalForceY * 0.01f;
}

// Remove the UpdateEntity function from pathfinding.c
// It should be defined in entity.c instead