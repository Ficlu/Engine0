// pathfinding.c

#include "pathfinding.h"
#include "entity.h"
#include "grid.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <float.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>



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
    int dx = abs(x1 - x2);
    int dy = abs(y1 - y2);
    return (dx + dy) + (1.414f - 2) * fminf(dx, dy);
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
void destroyPriorityQueue(PriorityQueue* pq) {
    if (pq) {
        free(pq->nodes);
        free(pq);
    }
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
Node* findPath(int startX, int startY, int goalX, int goalY, int* pathLength) {
    PriorityQueue* openList = createPriorityQueue(GRID_SIZE * GRID_SIZE);
    bool** closedList = (bool**)malloc(sizeof(bool*) * GRID_SIZE);
    for (int i = 0; i < GRID_SIZE; i++) {
        closedList[i] = (bool*)calloc(GRID_SIZE, sizeof(bool));
    }
    Node* nodes = (Node*)malloc(sizeof(Node) * GRID_SIZE * GRID_SIZE);

    if (!openList || !closedList || !nodes) {
        // Handle memory allocation failure
        if (openList) destroyPriorityQueue(openList);
        if (closedList) {
            for (int i = 0; i < GRID_SIZE; i++) {
                if (closedList[i]) free(closedList[i]);
            }
            free(closedList);
        }
        if (nodes) free(nodes);
        *pathLength = 0;
        return NULL;
    }

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

    bool pathFound = false;
    Node* endNode = NULL;

    while (openList->size > 0) {
        Node current = pop(openList);

        if (current.x == goalX && current.y == goalY) {
            pathFound = true;
            endNode = &nodes[current.y * GRID_SIZE + current.x];
            break;
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

    // Clean up
    for (int i = 0; i < GRID_SIZE; i++) {
        free(closedList[i]);
    }
    free(closedList);
    destroyPriorityQueue(openList);

    if (!pathFound) {
        free(nodes);
        *pathLength = 0;
        return NULL;
    }

    // Count path length
    *pathLength = 0;
    Node* current = endNode;
    while (current) {
        (*pathLength)++;
        current = current->parent;
    }

    // Allocate memory for the path
    Node* path = (Node*)malloc(sizeof(Node) * (*pathLength));
    if (!path) {
        free(nodes);
        *pathLength = 0;
        return NULL;
    }

    // Fill the path array
    current = endNode;
    for (int i = *pathLength - 1; i >= 0; i--) {
        path[i] = *current;
        current = current->parent;
    }

    free(nodes);
    return path;
}

// New GPU-based A* implementation

#define WORK_GROUP_SIZE 256

const char* computeShaderSource = 
"#version 430 core\n"
"\n"
"struct GPUNode {\n"
"    ivec2 pos;\n"
"    float g;\n"
"    float h;\n"
"    float f;\n"
"    ivec2 parent;\n"
"};\n"
"\n"
"layout(local_size_x = 256) in;\n"
"\n"
"layout(std430, binding = 0) buffer GridBuffer {\n"
"    int grid[];\n"
"};\n"
"\n"
"layout(std430, binding = 1) buffer OpenListBuffer {\n"
"    GPUNode openList[];\n"
"};\n"
"\n"
"layout(std430, binding = 2) buffer ClosedListBuffer {\n"
"    GPUNode closedList[];\n"
"};\n"
"\n"
"layout(std430, binding = 3) buffer PathBuffer {\n"
"    ivec2 path[];\n"
"};\n"
"\n"
"uniform ivec2 gridSize;\n"
"uniform ivec2 startPos;\n"
"uniform ivec2 goalPos;\n"
"uniform int maxIterations;\n"
"\n"
"shared GPUNode sharedOpenList[256];\n"
"shared int openListSize;\n"
"shared bool pathFound;\n"
"\n"
"float heuristic(ivec2 a, ivec2 b) {\n"
"    ivec2 diff = abs(a - b);\n"
"    return sqrt(float(diff.x * diff.x + diff.y * diff.y));\n"
"}\n"
"\n"
"void main() {\n"
"    uint gid = gl_GlobalInvocationID.x;\n"
"    uint lid = gl_LocalInvocationID.x;\n"
"\n"
"    if (gid == 0) {\n"
"        openListSize = 1;\n"
"        pathFound = false;\n"
"        openList[0] = GPUNode(startPos, 0.0, heuristic(startPos, goalPos), heuristic(startPos, goalPos), ivec2(-1, -1));\n"
"    }\n"
"\n"
"    barrier();\n"
"\n"
"    for (int iteration = 0; iteration < maxIterations && !pathFound; ++iteration) {\n"
"        // ... (rest of the compute shader code, replace Node with GPUNode)\n"
"    }\n"
"}\n";

GLuint computeShaderProgram;
GLuint gridBuffer;
GLuint openListBuffer;
GLuint closedListBuffer;
GLuint pathBuffer;


void initializeGPUPathfinding() {
    // Create and compile the compute shader
    GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(computeShader, 1, &computeShaderSource, NULL);
    glCompileShader(computeShader);

    // Check for compilation errors
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(computeShader, 512, NULL, infoLog);
        printf("Compute shader compilation failed: %s\n", infoLog);
    }

    // Create the compute shader program
    computeShaderProgram = glCreateProgram();
    glAttachShader(computeShaderProgram, computeShader);
    glLinkProgram(computeShaderProgram);

    // Check for linking errors
    glGetProgramiv(computeShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(computeShaderProgram, 512, NULL, infoLog);
        printf("Compute shader program linking failed: %s\n", infoLog);
    }

    glDeleteShader(computeShader);

    // Create buffers
    glGenBuffers(1, &gridBuffer);
    glGenBuffers(1, &openListBuffer);
    glGenBuffers(1, &closedListBuffer);
    glGenBuffers(1, &pathBuffer);

    // Initialize buffers
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gridBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, GRID_SIZE * GRID_SIZE * sizeof(int), NULL, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, openListBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, GRID_SIZE * GRID_SIZE * sizeof(GPUNode), NULL, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, closedListBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, GRID_SIZE * GRID_SIZE * sizeof(GPUNode), NULL, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, pathBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, GRID_SIZE * GRID_SIZE * sizeof(int) * 2, NULL, GL_DYNAMIC_READ);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

Node* findPathGPU(int startX, int startY, int goalX, int goalY, int* pathLength) {
    // Update grid buffer
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gridBuffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, GRID_SIZE * GRID_SIZE * sizeof(int), grid);

    // Set uniforms
    glUseProgram(computeShaderProgram);
    glUniform2i(glGetUniformLocation(computeShaderProgram, "gridSize"), GRID_SIZE, GRID_SIZE);
    glUniform2i(glGetUniformLocation(computeShaderProgram, "startPos"), startX, startY);
    glUniform2i(glGetUniformLocation(computeShaderProgram, "goalPos"), goalX, goalY);
    glUniform1i(glGetUniformLocation(computeShaderProgram, "maxIterations"), GRID_SIZE * GRID_SIZE);

    // Bind buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gridBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, openListBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, closedListBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, pathBuffer);

    // Dispatch compute shader
    glDispatchCompute(GRID_SIZE * GRID_SIZE / 256 + 1, 1, 1);

    // Wait for the compute shader to finish
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Read back the path
    int* pathData = (int*)malloc(GRID_SIZE * GRID_SIZE * 2 * sizeof(int));
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, pathBuffer);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, GRID_SIZE * GRID_SIZE * 2 * sizeof(int), pathData);

    // Count the path length
    *pathLength = 0;
    while (pathData[*pathLength * 2] != startX || pathData[*pathLength * 2 + 1] != startY) {
        (*pathLength)++;
        if (*pathLength >= GRID_SIZE * GRID_SIZE) {
            free(pathData);
            return NULL; // Path not found
        }
    }
    (*pathLength)++; // Include the start position

    // Create the path array using the CPU Node struct
    Node* path = (Node*)malloc(*pathLength * sizeof(Node));
    for (int i = 0; i < *pathLength; i++) {
        path[i].x = pathData[(*pathLength - 1 - i) * 2];
        path[i].y = pathData[(*pathLength - 1 - i) * 2 + 1];
        path[i].parent = (i < *pathLength - 1) ? &path[i + 1] : NULL;
        // Note: g, h, and f values are not set here as they're not needed for the final path
    }

    free(pathData);
    return path;
}
void cleanupGPUPathfinding() {
    glDeleteProgram(computeShaderProgram);
    glDeleteBuffers(1, &gridBuffer);
    glDeleteBuffers(1, &openListBuffer);
    glDeleteBuffers(1, &closedListBuffer);
    glDeleteBuffers(1, &pathBuffer);
}