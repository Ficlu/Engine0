#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <math.h>
#include "rendering.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define GRID_SIZE 20
#define TILE_SIZE (2.0f / GRID_SIZE)  // Size of a tile in normalized device coordinates
#define MOVE_SPEED 0.05f
#define GAME_LOGIC_INTERVAL_MS 600
#define TARGET_FPS 200
#define FRAME_TIME_MS (1000 / TARGET_FPS)

// Forward declarations
void GameLoop();
void Initialize();
void HandleInput();
void UpdateGameLogic();
void Render();
void CleanUp();
int PhysicsLoop(void* arg);

atomic_bool isRunning = true;
SDL_Window* window = NULL;
SDL_GLContext mainContext;
GLuint shaderProgram;
GLuint gridVAO;
GLuint squareVAO;
GLuint squareVBO;
int vertexCount;

typedef struct {
    bool walkable;
} Tile;

Tile grid[GRID_SIZE][GRID_SIZE];

// Square position
float squarePosX = 0.0f, squarePosY = 0.0f;
float targetPosX = 0.0f, targetPosY = 0.0f;

void GameLoop() {
    Initialize();

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
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        exit(1);
    }

    window = SDL_CreateWindow("2D Top-Down RPG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        exit(1);
    }

    mainContext = SDL_GL_CreateContext(window);
    if (!mainContext) {
        printf("SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_GL_MakeCurrent(window, mainContext);
    SDL_GL_SetSwapInterval(1); // Enable V-Sync

    glewExperimental = GL_TRUE; 
    if (glewInit() != GLEW_OK) {
        printf("Error initializing GLEW\n");
        exit(1);
    }

    shaderProgram = createShaderProgram();
    if (!shaderProgram) {
        printf("Failed to create shader program\n");
        exit(1);
    }

    float* vertices;
    createGridVertices(&vertices, &vertexCount, 800, 800, 800 / GRID_SIZE);
    gridVAO = createGridVAO(vertices, vertexCount);
    if (!gridVAO) {
        printf("Failed to create grid VAO\n");
        exit(1);
    }
    free(vertices);

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

    // Initialize grid
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            grid[y][x].walkable = (rand() % 100 < 80);  // 80% chance of being walkable
        }
    }
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

                // Ensure the clicked tile is within the grid bounds
                if (gridX >= 0 && gridX < GRID_SIZE && gridY >= 0 && gridY < GRID_SIZE) {
                    // Set target position to the center of the clicked tile
                    targetPosX = (2.0f * gridX / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE);
                    targetPosY = 1.0f - (2.0f * gridY / GRID_SIZE) - (1.0f / GRID_SIZE);

                    printf("Tile Clicked: gridX = %d, gridY = %d, targetPosX = %f, targetPosY = %f\n", gridX, gridY, targetPosX, targetPosY);
                }
            }
        }
    }
}
void UpdateGameLogic() {
    // Game logic updates can be placed here
}

void Render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);

    // Render grid
    glBindVertexArray(gridVAO);
    glDrawArrays(GL_LINES, 0, vertexCount / 2);
    glBindVertexArray(0);

    // Render square at its current position
    float squareVertices[] = {
        squarePosX - TILE_SIZE / 2, squarePosY - TILE_SIZE / 2,
        squarePosX + TILE_SIZE / 2, squarePosY - TILE_SIZE / 2,
        squarePosX + TILE_SIZE / 2, squarePosY + TILE_SIZE / 2,
        squarePosX - TILE_SIZE / 2, squarePosY + TILE_SIZE / 2
    };

    glBindVertexArray(squareVAO);
    glBindBuffer(GL_ARRAY_BUFFER, squareVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(squareVertices), squareVertices);
    glDrawArrays(GL_QUADS, 0, 4);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    SDL_GL_SwapWindow(window);
}

int PhysicsLoop(void* arg) {
    (void)arg;
    while (atomic_load(&isRunning)) {
        Uint32 startTime = SDL_GetTicks();

        // Calculate the direction vector
        float dx = targetPosX - squarePosX;
        float dy = targetPosY - squarePosY;
        
        // Calculate the distance to the target
        float distance = sqrt(dx*dx + dy*dy);

        if (distance > MOVE_SPEED) {
            // Normalize the direction vector
            float normalizedDx = dx / distance;
            float normalizedDy = dy / distance;

            // Move the square
            squarePosX += normalizedDx * MOVE_SPEED;
            squarePosY += normalizedDy * MOVE_SPEED;
        } else {
            // If we're closer than MOVE_SPEED, snap to the target position
            squarePosX = targetPosX;
            squarePosY = targetPosY;
        }

        int interval = 8 - (SDL_GetTicks() - startTime);
        if (interval > 0) {
            SDL_Delay(interval);
        }
    }
    return 0;
}

void CleanUp() {
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
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    GameLoop();
    return 0;
}