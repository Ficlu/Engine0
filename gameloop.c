#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdatomic.h>
#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define GRID_SIZE 20
#define TILE_SIZE (2.0f / GRID_SIZE)
#define MAX_PATH_LENGTH (GRID_SIZE * GRID_SIZE)

// Function declarations
void Initialize();
void HandleInput();
int PhysicsLoop(void* arg);
int RenderLoop(void* arg);
void UpdateGameLogic();
void CleanUp();
void CreateSquare(GLuint* VAO, GLuint* VBO, GLuint* EBO, GLuint* shaderProgram);
void InitFramebuffers();
void InitializeGrid();
void AStarPathfinding(int startX, int startY, int goalX, int goalY);
int heuristic(int x1, int y1, int x2, int y2);
bool isValid(int x, int y);
bool isUnBlocked(int x, int y);
bool isDestination(int x, int y, int destX, int destY);
void CreateGrid();
void RenderGrid();

typedef struct {
    bool walkable;
} Tile;

typedef struct {
    int x, y;
    int f, g, h;
    int parentX, parentY;
} Node;

bool isRunning = true;
SDL_Window* window = NULL;
SDL_GLContext mainContext;
SDL_sem* initSemaphore;

GLuint framebuffers[3];
GLuint framebufferTextures[3];
int currentBufferIndex = 0;

// Game state
atomic_int squareX = ATOMIC_VAR_INIT(0);
atomic_int squareY = ATOMIC_VAR_INIT(0);
atomic_int targetX = ATOMIC_VAR_INIT(0);
atomic_int targetY = ATOMIC_VAR_INIT(0);
atomic_bool hasTarget = ATOMIC_VAR_INIT(false);

Tile grid[GRID_SIZE][GRID_SIZE];
Node* currentPath = NULL;
int currentPathLength = 0;
int currentPathCapacity = 0;
int currentPathIndex = 0;

GLuint gridVAO, gridVBO;
GLuint gridShaderProgram;

const int TARGET_FPS = 200;
const int TARGET_FRAME_TIME = 1000 / TARGET_FPS;
const int LOGIC_UPDATE_INTERVAL = 16; // ~60 updates per second
const int MIN_PHYSICS_INTERVAL = 8; // ~120 physics updates per second
const int SQUARE_SPEED = 5; // tiles per second

const char* vertexShaderSource = "#version 330 core\n"
"layout(location = 0) in vec3 aPos;\n"
"layout(location = 1) in vec3 aColor;\n"
"out vec3 ourColor;\n"
"uniform vec2 uOffset;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos.x + uOffset.x, aPos.y + uOffset.y, aPos.z, 1.0);\n"
"   ourColor = aColor;\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"in vec3 ourColor;\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(ourColor, 1.0);\n"
"}\n\0";

void GameLoop() {
    Initialize();

    SDL_Thread* physicsThread = SDL_CreateThread(PhysicsLoop, "PhysicsThread", NULL);
    SDL_Thread* renderThread = SDL_CreateThread(RenderLoop, "RenderThread", NULL);

    Uint32 lastTickTime = SDL_GetTicks();

    while (isRunning) {
        Uint32 currentTime = SDL_GetTicks();
        if ((currentTime - lastTickTime) >= (Uint32)LOGIC_UPDATE_INTERVAL) {
            HandleInput();
            UpdateGameLogic();
            lastTickTime = currentTime;
        }

        SDL_Delay(1); // minimal delay to yield CPU
    }

    CleanUp();
    SDL_WaitThread(physicsThread, NULL);
    SDL_WaitThread(renderThread, NULL);
}

void Initialize() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    SDL_GL_SetSwapInterval(1); // Enable V-Sync

    window = SDL_CreateWindow("2D Top-Down RPG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    mainContext = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, mainContext);

    glewExperimental = GL_TRUE;
    glewInit();

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    initSemaphore = SDL_CreateSemaphore(0);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    InitFramebuffers();
    InitializeGrid();
    CreateGrid();

    // Initialize square position to the center of the grid
    atomic_store(&squareX, (GRID_SIZE / 2) * (1000 / GRID_SIZE));
    atomic_store(&squareY, (GRID_SIZE / 2) * (1000 / GRID_SIZE));
}

void InitFramebuffers() {
    glGenFramebuffers(3, framebuffers);
    glGenTextures(3, framebufferTextures);

    for (int i = 0; i < 3; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[i]);

        glBindTexture(GL_TEXTURE_2D, framebufferTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferTextures[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            printf("Error initializing framebuffer %d\n", i);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void InitializeGrid() {
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            grid[y][x].walkable = (rand() % 100 < 80);  // 80% chance of being walkable
        }
    }
    // Ensure starting position is walkable
    grid[GRID_SIZE/2][GRID_SIZE/2].walkable = true;
}

void CreateSquare(GLuint* VAO, GLuint* VBO, GLuint* EBO, GLuint* shaderProgram) {
    float vertices[] = {
         0.1f,  0.1f, 0.0f, 1.0f, 0.0f, 0.0f,
         0.1f, -0.1f, 0.0f, 0.0f, 1.0f, 0.0f,
        -0.1f, -0.1f, 0.0f, 0.0f, 0.0f, 1.0f,
        -0.1f,  0.1f, 0.0f, 1.0f, 1.0f, 0.0f
    };
    unsigned int indices[] = {
        0, 1, 3,
        1, 2, 3
    };

    glGenVertexArrays(1, VAO);
    glBindVertexArray(*VAO);

    glGenBuffers(1, VBO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    *shaderProgram = glCreateProgram();
    glAttachShader(*shaderProgram, vertexShader);
    glAttachShader(*shaderProgram, fragmentShader);
    glLinkProgram(*shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void CreateGrid() {
    float* vertices = malloc(GRID_SIZE * GRID_SIZE * 6 * 5 * sizeof(float));
    int vertexCount = 0;

    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            float xPos = x * TILE_SIZE - 1.0f;
            float yPos = y * TILE_SIZE - 1.0f;

            // Define the four corners of the tile
            float tile[6][5] = {
                {xPos, yPos, 0.0f, 0.2f, 0.2f},
                {xPos + TILE_SIZE, yPos, 0.0f, 0.2f, 0.2f},
                {xPos, yPos + TILE_SIZE, 0.0f, 0.2f, 0.2f},
                {xPos + TILE_SIZE, yPos, 0.0f, 0.2f, 0.2f},
                {xPos + TILE_SIZE, yPos + TILE_SIZE, 0.0f, 0.2f, 0.2f},
                {xPos, yPos + TILE_SIZE, 0.0f, 0.2f, 0.2f}
            };

            memcpy(&vertices[vertexCount * 5], tile, 6 * 5 * sizeof(float));
            vertexCount += 6;
        }
    }

    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);

    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * 5 * sizeof(float), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    free(vertices);

    // Create and compile the grid shader program
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    gridShaderProgram = glCreateProgram();
    glAttachShader(gridShaderProgram, vertexShader);
    glAttachShader(gridShaderProgram, fragmentShader);
    glLinkProgram(gridShaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void RenderGrid() {
    glUseProgram(gridShaderProgram);
    glBindVertexArray(gridVAO);
    glDrawArrays(GL_TRIANGLES, 0, GRID_SIZE * GRID_SIZE * 6);
    glBindVertexArray(0);
    glUseProgram(0);
}

void HandleInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            isRunning = false;
        } else if (event.type == SDL_MOUSEBUTTONDOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);
                
                // Convert screen coordinates to grid coordinates
                int gridX = (int)((float)mouseX / WINDOW_WIDTH * GRID_SIZE);
                int gridY = GRID_SIZE - 1 - (int)((float)mouseY / WINDOW_HEIGHT * GRID_SIZE);
                
                if (gridX >= 0 && gridX < GRID_SIZE && gridY >= 0 && gridY < GRID_SIZE && grid[gridY][gridX].walkable) {
                    int startX = atomic_load(&squareX) / (1000 / GRID_SIZE);
int startY = atomic_load(&squareY) / (1000 / GRID_SIZE);
                    AStarPathfinding(startX, startY, gridX, gridY);
                }
            }
        }
    }
}

void UpdateGameLogic() {
    // Update game logic here if needed
}

int PhysicsLoop(void* arg) {
    (void)arg;
    while (isRunning) {
        Uint32 startTime = SDL_GetTicks();
        
        if (atomic_load(&hasTarget) && currentPathIndex < currentPathLength) {
            int currentX = atomic_load(&squareX) / (1000 / GRID_SIZE);
            int currentY = atomic_load(&squareY) / (1000 / GRID_SIZE);
            
            int targetX = currentPath[currentPathIndex].x;
            int targetY = currentPath[currentPathIndex].y;
            
            if (currentX == targetX && currentY == targetY) {
                currentPathIndex++;
                if (currentPathIndex >= currentPathLength) {
                    atomic_store(&hasTarget, false);
                }
            } else {
                int dx = targetX - currentX;
                int dy = targetY - currentY;
                
                if (dx != 0) dx = dx > 0 ? 1 : -1;
                if (dy != 0) dy = dy > 0 ? 1 : -1;
                
                int newX = currentX + dx;
                int newY = currentY + dy;
                
                atomic_store(&squareX, newX * (1000 / GRID_SIZE));
                atomic_store(&squareY, newY * (1000 / GRID_SIZE));
            }
        }

        int interval = MIN_PHYSICS_INTERVAL - (SDL_GetTicks() - startTime);
        if (interval > 0) {
            SDL_Delay(interval);
        }
    }
    return 0;
}

int RenderLoop(void* arg) {
    (void)arg;
    
    SDL_GLContext renderContext = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, renderContext);

    GLuint VAO, VBO, EBO, shaderProgram;
    CreateSquare(&VAO, &VBO, &EBO, &shaderProgram);

    GLint offsetUniformLocation = glGetUniformLocation(shaderProgram, "uOffset");

    SDL_SemPost(initSemaphore);

    Uint32 frameStart, frameTime;
    Uint32 fpsLastTime = SDL_GetTicks();
    int frameCount = 0;

    while (isRunning) {
        frameStart = SDL_GetTicks();

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[currentBufferIndex]);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render the grid
        RenderGrid();

        // Render the square
        glUseProgram(shaderProgram);
        glUniform2f(offsetUniformLocation, 
                    (float)atomic_load(&squareX) / 500.0f - 1.0f, 
                    (float)atomic_load(&squareY) / 500.0f - 1.0f);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glUseProgram(0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, framebufferTextures[currentBufferIndex]);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        SDL_GL_SwapWindow(window);

        currentBufferIndex = (currentBufferIndex + 1) % 3;

        frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < (Uint32)TARGET_FRAME_TIME) {
            SDL_Delay(TARGET_FRAME_TIME - frameTime);
        }

        frameCount++;
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - fpsLastTime >= 5000) {  // Every 5 seconds
            float fps = frameCount / ((currentTime - fpsLastTime) / 1000.0f);
            printf("FPS: %.2f\n", fps);
            frameCount = 0;
            fpsLastTime = currentTime;
        }
    }
    
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    SDL_GL_DeleteContext(renderContext);
    return 0;
}

void CleanUp() {
    free(currentPath);
    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);
    glDeleteProgram(gridShaderProgram);
    SDL_GL_DeleteContext(mainContext);
    SDL_DestroyWindow(window);
    SDL_DestroySemaphore(initSemaphore);
    SDL_Quit();
}

int heuristic(int x1, int y1, int x2, int y2) {
    return abs(x1 - x2) + abs(y1 - y2);
}

bool isValid(int x, int y) {
    return (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE);
}

bool isUnBlocked(int x, int y) {
    return grid[y][x].walkable;
}

bool isDestination(int x, int y, int destX, int destY) {
    return (x == destX && y == destY);
}

void AStarPathfinding(int startX, int startY, int goalX, int goalY) {
    if (!isValid(startX, startY) || !isValid(goalX, goalY)) {
        printf("Invalid start or goal position\n");
        return;
    }

    if (!isUnBlocked(startX, startY) || !isUnBlocked(goalX, goalY)) {
        printf("Start or goal is blocked\n");
        return;
    }

    if (isDestination(startX, startY, goalX, goalY)) {
        printf("Already at destination\n");
        return;
    }

    bool* closedList = calloc(GRID_SIZE * GRID_SIZE, sizeof(bool));
    if (!closedList) {
        printf("Memory allocation failed for closedList\n");
        return;
    }

    Node* nodes = calloc(GRID_SIZE * GRID_SIZE, sizeof(Node));
    if (!nodes) {
        printf("Memory allocation failed for nodes\n");
        free(closedList);
        return;
    }

    for (int i = 0; i < GRID_SIZE * GRID_SIZE; i++) {
        nodes[i].f = nodes[i].g = nodes[i].h = INT_MAX;
        nodes[i].parentX = nodes[i].parentY = -1;
    }

    int x = startX, y = startY;
    nodes[y * GRID_SIZE + x].f = nodes[y * GRID_SIZE + x].g = nodes[y * GRID_SIZE + x].h = 0;
    nodes[y * GRID_SIZE + x].parentX = x;
    nodes[y * GRID_SIZE + x].parentY = y;

    Node* openList = malloc(MAX_PATH_LENGTH * sizeof(Node));
    if (!openList) {
        printf("Memory allocation failed for openList\n");
        free(closedList);
        free(nodes);
        return;
    }

    int openListSize = 0;
    openList[openListSize++] = (Node){x, y, 0, 0, 0, x, y};

    bool foundDest = false;

    while (openListSize > 0) {
        int current = 0;
        for (int i = 1; i < openListSize; i++) {
            if (openList[i].f < openList[current].f) {
                current = i;
            }
        }

        Node node = openList[current];
        openList[current] = openList[--openListSize];

        x = node.x;
        y = node.y;
        closedList[y * GRID_SIZE + x] = true;

        for (int newX = -1; newX <= 1; newX++) {
            for (int newY = -1; newY <= 1; newY++) {
                if (newX == 0 && newY == 0) continue;
                
                int adjX = x + newX;
                int adjY = y + newY;

                if (isValid(adjX, adjY)) {
                    if (isDestination(adjX, adjY, goalX, goalY)) {
                        nodes[adjY * GRID_SIZE + adjX].parentX = x;
                        nodes[adjY * GRID_SIZE + adjX].parentY = y;
                        foundDest = true;
                        break;
                    }
                    else if (!closedList[adjY * GRID_SIZE + adjX] && isUnBlocked(adjX, adjY)) {
                        int newG = nodes[y * GRID_SIZE + x].g + 1;
                        int newH = heuristic(adjX, adjY, goalX, goalY);
                        int newF = newG + newH;

                        if (nodes[adjY * GRID_SIZE + adjX].f == INT_MAX || 
                            nodes[adjY * GRID_SIZE + adjX].f > newF) {
                            nodes[adjY * GRID_SIZE + adjX].f = newF;
                            nodes[adjY * GRID_SIZE + adjX].g = newG;
                            nodes[adjY * GRID_SIZE + adjX].h = newH;
                            nodes[adjY * GRID_SIZE + adjX].parentX = x;
                            nodes[adjY * GRID_SIZE + adjX].parentY = y;

                            if (openListSize < MAX_PATH_LENGTH) {
                                openList[openListSize++] = (Node){adjX, adjY, newF, newG, newH, x, y};
                            }
                        }
                    }
                }
            }
            if (foundDest) break;
        }
        if (foundDest) break;
    }

    if (foundDest) {
        Node* newPath = malloc(MAX_PATH_LENGTH * sizeof(Node));
        if (!newPath) {
            printf("Memory allocation failed for newPath\n");
        } else {
            int pathLength = 0;
            int cx = goalX, cy = goalY;
            while (!(nodes[cy * GRID_SIZE + cx].parentX == cx && 
                     nodes[cy * GRID_SIZE + cx].parentY == cy)) {
                newPath[pathLength++] = (Node){cx, cy, 0, 0, 0, 0, 0};
                int tempX = nodes[cy * GRID_SIZE + cx].parentX;
                int tempY = nodes[cy * GRID_SIZE + cx].parentY;
                cx = tempX;
                cy = tempY;
            }
            newPath[pathLength++] = (Node){cx, cy, 0, 0, 0, 0, 0};

            // Reverse the path
            for (int i = 0; i < pathLength / 2; i++) {
                Node temp = newPath[i];
                newPath[i] = newPath[pathLength - 1 - i];
                newPath[pathLength - 1 - i] = temp;
            }

            // Update the current path
            free(currentPath);
            currentPath = newPath;
            currentPathLength = pathLength;
            currentPathCapacity = MAX_PATH_LENGTH;
            currentPathIndex = 0;
            atomic_store(&hasTarget, true);
        }
    } else {
        printf("Failed to find the destination cell\n");
    }

    free(closedList);
    free(nodes);
    free(openList);
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    GameLoop();
    return 0;
}