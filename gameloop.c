// gameloop.c
#include "gameloop.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#include <stdatomic.h>
#include "rendering.h"
#include "player.h"
#include "enemy.h"
#include "grid.h"
#include "entity.h"
#include "pathfinding.h"

atomic_bool isRunning = true;
SDL_Window* window = NULL;
SDL_GLContext mainContext;
GLuint shaderProgram;
GLuint gridVAO;
GLuint squareVAO;
GLuint squareVBO;
int vertexCount;
GLuint colorUniform;

Player player;
Enemy enemies[MAX_ENEMIES];

void GameLoop() {
    Initialize();
    printf("Entering main game loop.\n");

    SDL_Thread* physicsThread = SDL_CreateThread(PhysicsLoop, "PhysicsThread", NULL);

    Uint32 lastLogicTick = SDL_GetTicks();
    Uint32 lastRenderTick = SDL_GetTicks();

    while (atomic_load(&isRunning)) {
        Uint32 currentTick = SDL_GetTicks();

        if ((currentTick - lastLogicTick) >= GAME_LOGIC_INTERVAL_MS) {
            UpdateGameLogic();
            lastLogicTick = currentTick;
        }

        if ((currentTick - lastRenderTick) >= FRAME_TIME_MS) {
            HandleInput();
            Render();
            lastRenderTick = currentTick;
        }

        SDL_Delay(1); // minimal delay to yield CPU
    }

    SDL_WaitThread(physicsThread, NULL);
    CleanUp();
}

void Initialize() {
    printf("Initializing...\n");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        exit(1);
    }
    printf("SDL initialized.\n");

    window = SDL_CreateWindow("2D Top-Down RPG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        exit(1);
    }
    printf("Window created.\n");

    mainContext = SDL_GL_CreateContext(window);
    if (!mainContext) {
        printf("SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        exit(1);
    }
    printf("OpenGL context created.\n");

    SDL_GL_MakeCurrent(window, mainContext);
    SDL_GL_SetSwapInterval(1); // Enable V-Sync

    glewExperimental = GL_TRUE; 
    if (glewInit() != GLEW_OK) {
        printf("Error initializing GLEW\n");
        exit(1);
    }
    printf("GLEW initialized.\n");

    shaderProgram = createShaderProgram();
    if (!shaderProgram) {
        printf("Failed to create shader program\n");
        exit(1);
    }
    printf("Shader program created.\n");

    colorUniform = glGetUniformLocation(shaderProgram, "color");

    float* vertices;
    createGridVertices(&vertices, &vertexCount, 800, 800, 800 / GRID_SIZE);
    gridVAO = createGridVAO(vertices, vertexCount);
    if (!gridVAO) {
        printf("Failed to create grid VAO\n");
        exit(1);
    }
    free(vertices);
    printf("Grid VAO created.\n");

    // Create square VAO and VBO
    glGenVertexArrays(1, &squareVAO);
    glGenBuffers(1, &squareVBO);

    glBindVertexArray(squareVAO);

    float squareVertices[] = {
        -TILE_SIZE / 2, -TILE_SIZE / 2,
         TILE_SIZE / 2, -TILE_SIZE / 2,
         TILE_SIZE / 2,  TILE_SIZE / 2,
        -TILE_SIZE / 2,  TILE_SIZE / 2
    };

    glBindBuffer(GL_ARRAY_BUFFER, squareVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), squareVertices, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    printf("Square VAO and VBO created.\n");

    // Initialize grid with some unwalkable tiles
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            grid[y][x].walkable = (rand() % 10 < 8);  // 80% chance of being walkable
        }
    }

    // Ensure there's at least one walkable path across the grid
    for (int x = 0; x < GRID_SIZE; x++) {
        grid[GRID_SIZE / 2][x].walkable = true;
    }
    printf("Grid initialized.\n");

    // Initialize player on a walkable tile
    int playerGridX, playerGridY;
    do {
        playerGridX = rand() % GRID_SIZE;
        playerGridY = rand() % GRID_SIZE;
    } while (!grid[playerGridY][playerGridX].walkable);

    InitPlayer(&player, playerGridX, playerGridY, MOVE_SPEED);
    printf("Player initialized at (%d, %d).\n", playerGridX, playerGridY);

    // Initialize enemies on walkable tiles
    for (int i = 0; i < MAX_ENEMIES; i++) {
        int enemyGridX, enemyGridY;
        do {
            enemyGridX = rand() % GRID_SIZE;
            enemyGridY = rand() % GRID_SIZE;
        } while (!grid[enemyGridY][enemyGridX].walkable || 
                 (enemyGridX == playerGridX && enemyGridY == playerGridY));

        InitEnemy(&enemies[i], enemyGridX, enemyGridY, MOVE_SPEED * 0.5f);
        printf("Enemy %d initialized at (%d, %d).\n", i, enemyGridX, enemyGridY);
    }

    printf("Initialization complete.\n");
}

void HandleInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            atomic_store(&isRunning, false);
        } else if (event.type == SDL_MOUSEBUTTONDOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);

                // Convert screen coordinates to grid coordinates
                int gridX = mouseX / (WINDOW_WIDTH / GRID_SIZE);
                int gridY = mouseY / (WINDOW_HEIGHT / GRID_SIZE);

                // Ensure the clicked tile is within the grid bounds and walkable
                if (gridX >= 0 && gridX < GRID_SIZE && gridY >= 0 && gridY < GRID_SIZE && isWalkable(gridX, gridY)) {
                    player.entity.finalGoalX = gridX;
                    player.entity.finalGoalY = gridY;
                    player.entity.targetGridX = player.entity.gridX;  // Start from current position
                    player.entity.targetGridY = player.entity.gridY;
                    player.entity.needsPathfinding = true;

                    printf("Player final goal set: gridX = %d, gridY = %d\n", gridX, gridY);
                }
            }
        }
    }
}

void UpdateGameLogic() {
    Entity* allEntities[MAX_ENEMIES + 1];
    allEntities[0] = &player.entity;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        allEntities[i + 1] = &enemies[i].entity;
    }

    UpdatePlayer(&player, allEntities, MAX_ENEMIES + 1);

    // Update enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        UpdateEnemy(&enemies[i], allEntities, MAX_ENEMIES + 1);
    }
}

void Render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);

    // Render tiles
    glBindVertexArray(squareVAO);
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            float posX = (2.0f * x / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE);
            float posY = 1.0f - (2.0f * y / GRID_SIZE) - (1.0f / GRID_SIZE);

            if (!grid[y][x].walkable) {
                glUniform4f(colorUniform, 0.7f, 0.2f, 0.2f, 1.0f);  // Red for unwalkable
            } else {
                glUniform4f(colorUniform, 0.2f, 0.7f, 0.2f, 1.0f);  // Green for walkable
            }

            float tileVertices[] = {
                posX - TILE_SIZE / 2, posY - TILE_SIZE / 2,
                posX + TILE_SIZE / 2, posY - TILE_SIZE / 2,
                posX + TILE_SIZE / 2, posY + TILE_SIZE / 2,
                posX - TILE_SIZE / 2, posY + TILE_SIZE / 2
            };

            glBindBuffer(GL_ARRAY_BUFFER, squareVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(tileVertices), tileVertices);
            glDrawArrays(GL_QUADS, 0, 4);
        }
    }

    // Render grid
    glUniform4f(colorUniform, 1.0f, 1.0f, 1.0f, 1.0f);  // White for grid lines
    glBindVertexArray(gridVAO);
    glDrawArrays(GL_LINES, 0, vertexCount / 2);

    // Render paths (new code)
    glUniform4f(colorUniform, 1.0f, 1.0f, 0.0f, 0.5f);  // Semi-transparent yellow for paths
    glBindVertexArray(squareVAO);
    
    // Render player path
if (player.entity.cachedPath) {
    for (int i = 0; i < player.entity.cachedPathLength; i++) {
        float pathX = (2.0f * player.entity.cachedPath[i].x / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE);
        float pathY = 1.0f - (2.0f * player.entity.cachedPath[i].y / GRID_SIZE) - (1.0f / GRID_SIZE);
            float pathVertices[] = {
                pathX - TILE_SIZE / 4, pathY - TILE_SIZE / 4,
                pathX + TILE_SIZE / 4, pathY - TILE_SIZE / 4,
                pathX + TILE_SIZE / 4, pathY + TILE_SIZE / 4,
                pathX - TILE_SIZE / 4, pathY + TILE_SIZE / 4
            };
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pathVertices), pathVertices);
            glDrawArrays(GL_QUADS, 0, 4);
        }
    }

    // Render player
    glUniform4f(colorUniform, 0.0f, 0.0f, 1.0f, 1.0f);  // Blue for player
    float playerPosX = player.entity.posX;
    float playerPosY = player.entity.posY;
    float playerVertices[] = {
        playerPosX - TILE_SIZE / 2, playerPosY - TILE_SIZE / 2,
        playerPosX + TILE_SIZE / 2, playerPosY - TILE_SIZE / 2,
        playerPosX + TILE_SIZE / 2, playerPosY + TILE_SIZE / 2,
        playerPosX - TILE_SIZE / 2, playerPosY + TILE_SIZE / 2
    };

    glBindVertexArray(squareVAO);
    glBindBuffer(GL_ARRAY_BUFFER, squareVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(playerVertices), playerVertices);
    glDrawArrays(GL_QUADS, 0, 4);

    // Render enemies
    glUniform4f(colorUniform, 1.0f, 0.0f, 0.0f, 1.0f);  // Red for enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        float enemyPosX = enemies[i].entity.posX;
        float enemyPosY = enemies[i].entity.posY;
        float enemyVertices[] = {
            enemyPosX - TILE_SIZE / 2, enemyPosY - TILE_SIZE / 2,
            enemyPosX + TILE_SIZE / 2, enemyPosY - TILE_SIZE / 2,
            enemyPosX + TILE_SIZE / 2, enemyPosY + TILE_SIZE / 2,
            enemyPosX - TILE_SIZE / 2, enemyPosY + TILE_SIZE / 2
        };

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(enemyVertices), enemyVertices);
        glDrawArrays(GL_QUADS, 0, 4);

        // Render enemy path (new code)
if (enemies[i].entity.cachedPath) {
    for (int j = 0; j < enemies[i].entity.cachedPathLength; j++) {
        float pathX = (2.0f * enemies[i].entity.cachedPath[j].x / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE);
        float pathY = 1.0f - (2.0f * enemies[i].entity.cachedPath[j].y / GRID_SIZE) - (1.0f / GRID_SIZE);
            float pathVertices[] = {
                    pathX - TILE_SIZE / 4, pathY - TILE_SIZE / 4,
                    pathX + TILE_SIZE / 4, pathY - TILE_SIZE / 4,
                    pathX + TILE_SIZE / 4, pathY + TILE_SIZE / 4,
                    pathX - TILE_SIZE / 4, pathY + TILE_SIZE / 4
                };
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pathVertices), pathVertices);
                glDrawArrays(GL_QUADS, 0, 4);
            }
            glUniform4f(colorUniform, 1.0f, 0.0f, 0.0f, 1.0f);  // Red for enemies (reset color)
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    SDL_GL_SwapWindow(window);
}
int PhysicsLoop(void* arg) {
    (void)arg;
    while (atomic_load(&isRunning)) {
        Uint32 startTime = SDL_GetTicks();
        
        Entity* allEntities[MAX_ENEMIES + 1];
        allEntities[0] = &player.entity;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            allEntities[i + 1] = &enemies[i].entity;
        }

        UpdateEntity(&player.entity, allEntities, MAX_ENEMIES + 1);

        // Update enemies
        for (int i = 0; i < MAX_ENEMIES; i++) {
            UpdateEntity(&enemies[i].entity, allEntities, MAX_ENEMIES + 1);
        }

        int interval = 8 - (SDL_GetTicks() - startTime);
        if (interval > 0) {
            SDL_Delay(interval);
        }
    }
    return 0;
}

void CleanUp() {
    printf("Cleaning up...\n");
    if (gridVAO) {
        glDeleteVertexArrays(1, &gridVAO);
    }
    if (squareVAO) {
        glDeleteVertexArrays(1, &squareVAO);
    }
    if (squareVBO) {
        glDeleteBuffers(1, &squareVBO);
    }
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
    }

    if (mainContext) {
        SDL_GL_DeleteContext(mainContext);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
    printf("Cleanup complete.\n");
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    printf("Starting GameLoop...\n");
    GameLoop();
    printf("GameLoop ended.\n");
    return 0;
}