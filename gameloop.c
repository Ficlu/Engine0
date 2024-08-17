// gameloop.c

#include "gameloop.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#include <stdatomic.h>
#include <time.h>
#include "rendering.h"
#include "player.h"
#include "enemy.h"
#include "grid.h"
#include "entity.h"
#include "pathfinding.h"

#define UNWALKABLE_PROBABILITY 0.04f
GLuint outlineShaderProgram;
atomic_bool isRunning = true;
SDL_Window* window = NULL;
SDL_GLContext mainContext;
GLuint shaderProgram;
GLuint gridVAO;
GLuint squareVAO;
GLuint squareVBO;
int vertexCount;
GLuint colorUniform;

Entity* allEntities[MAX_ENTITIES];
Player player;
Enemy enemies[MAX_ENEMIES];

// Function declarations
void setGridSize(int size);
void generateTerrain();
void RenderEntities(float cameraOffsetX, float cameraOffsetY, float zoomFactor);
GLuint loadTexture(const char* filePath);  // New texture loading function

bool isPointVisible(float x, float y, float playerX, float playerY, float zoomFactor) {
    float minZoom = 2.0f;
    float visibleRadius = minZoom / zoomFactor;
    float dx = x - playerX;
    float dy = y - playerY;
    return (dx * dx + dy * dy) <= (visibleRadius * visibleRadius);
}

void WorldToScreenCoords(int gridX, int gridY, float cameraOffsetX, float cameraOffsetY, float zoomFactor, float* screenX, float* screenY) {
    *screenX = (2.0f * gridX / GRID_SIZE - 1.0f + 1.0f / GRID_SIZE - cameraOffsetX) * zoomFactor;
    *screenY = (1.0f - 2.0f * gridY / GRID_SIZE - 1.0f / GRID_SIZE - cameraOffsetY) * zoomFactor;
}

/*
 * GameLoop
 *
 * Main game loop function that handles initialization, rendering, and game logic updates.
 */
void GameLoop() {
    Initialize();
    printf("Entering main game loop.\n");

    SDL_Thread* physicsThread = SDL_CreateThread(PhysicsLoop, "PhysicsThread", NULL);
    srand((unsigned int)time(NULL));
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

        SDL_Delay(1);
    }

    SDL_WaitThread(physicsThread, NULL);
    CleanUp();
}

/*
 * Initialize
 *
 * Initializes SDL, OpenGL, game objects, and other resources needed for the game.
 */
void Initialize() {
    printf("Initializing...\n");

    setGridSize(40); // Set the grid size at the beginning of initialization
    generateTerrain(); // Generate the Minecraft-style terrain

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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glewExperimental = GL_TRUE; 
    if (glewInit() != GLEW_OK) {
        printf("Error initializing GLEW\n");
        exit(1);
    }
    printf("GLEW initialized.\n");

    shaderProgram = createShaderProgram();
    outlineShaderProgram = createOutlineShaderProgram();
    if (!shaderProgram || !outlineShaderProgram) {
        printf("Failed to create shader programs\n");
        exit(1);
    }
    printf("Shader programs created.\n");

    colorUniform = glGetUniformLocation(shaderProgram, "color");
    textureUniform = glGetUniformLocation(shaderProgram, "textureAtlas");

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
        -TILE_SIZE, -TILE_SIZE, 0.0f, 0.0f,
         TILE_SIZE, -TILE_SIZE, 1.0f, 0.0f,
         TILE_SIZE,  TILE_SIZE, 1.0f, 1.0f,
        -TILE_SIZE,  TILE_SIZE, 0.0f, 1.0f
    };

    glBindBuffer(GL_ARRAY_BUFFER, squareVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), squareVertices, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    printf("Square VAO and VBO created.\n");

    // Load texture atlas
    textureAtlas = loadBMP("texture_atlas.bmp");
    if (!textureAtlas) {
        printf("Failed to load texture atlas\n");
        exit(1);
    }

    // Initialize player on a walkable tile
    int playerGridX, playerGridY;
    do {
        playerGridX = rand() % GRID_SIZE;
        playerGridY = rand() % GRID_SIZE;
    } while (!grid[playerGridY][playerGridX].isWalkable);

    InitPlayer(&player, playerGridX, playerGridY, MOVE_SPEED);
    printf("Player initialized at (%d, %d).\n", playerGridX, playerGridY);

    // Initialize enemies on walkable tiles and set up allEntities
    allEntities[0] = &player.entity;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        int enemyGridX, enemyGridY;
        do {
            enemyGridX = rand() % GRID_SIZE;
            enemyGridY = rand() % GRID_SIZE;
        } while (!grid[enemyGridY][enemyGridX].isWalkable || 
                 (enemyGridX == playerGridX && enemyGridY == playerGridY));

        InitEnemy(&enemies[i], enemyGridX, enemyGridY, MOVE_SPEED * 0.5f);
        allEntities[i + 1] = &enemies[i].entity;
        printf("Enemy %d initialized at (%d, %d).\n", i, enemyGridX, enemyGridY);
    }

    printf("Initialization complete.\n");
}

void CleanupEntities() {
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (allEntities[i]) {
            if (i == 0) {  // The player is always at index 0
                CleanupPlayer(&player);
            } else {  // Enemies are from index 1 to MAX_ENEMIES
                CleanupEnemy(&enemies[i - 1]);
            }
        }
    }
}


void CleanUp() {
    printf("Cleaning up...\n");
    CleanupEntities();

    if (gridVAO) {
        glDeleteVertexArrays(1, &gridVAO);
    }
    if (squareVAO) {
        glDeleteVertexArrays(1, &squareVAO);
    }
    if (outlineVAO) {
        glDeleteVertexArrays(1, &outlineVAO);
    }
    if (squareVBO) {
        glDeleteBuffers(1, &squareVBO);
    }
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
    }
    if (outlineShaderProgram) {
        glDeleteProgram(outlineShaderProgram);
    }
    if (textureAtlas) {
        glDeleteTextures(1, &textureAtlas);
        textureAtlas = 0;  // Reset to 0 after deleting
    }
    if (mainContext) {
        SDL_GL_DeleteContext(mainContext);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    cleanupGrid();

    SDL_Quit();
    printf("Cleanup complete.\n");
}
/*
 * HandleInput
 *
 * Processes input events such as mouse clicks and keyboard input.
 */
void HandleInput() {
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            atomic_store(&isRunning, false);
        } else if (event.type == SDL_MOUSEBUTTONDOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);

                float ndcX = (2.0f * mouseX / WINDOW_WIDTH - 1.0f) / player.zoomFactor;
                float ndcY = (1.0f - 2.0f * mouseY / WINDOW_HEIGHT) / player.zoomFactor;

                float worldX = ndcX + player.cameraCurrentX;
                float worldY = ndcY + player.cameraCurrentY;

                int gridX = (int)((worldX + 1.0f) * GRID_SIZE / 2);
                int gridY = (int)((1.0f - worldY) * GRID_SIZE / 2);

                if (gridX >= 0 && gridX < GRID_SIZE && gridY >= 0 && gridY < GRID_SIZE && grid[gridY][gridX].isWalkable) {
                    player.entity.finalGoalX = gridX;
                    player.entity.finalGoalY = gridY;
                    player.entity.targetGridX = player.entity.gridX;
                    player.entity.targetGridY = player.entity.gridY;
                    player.entity.needsPathfinding = true;

                    printf("Player final goal set: gridX = %d, gridY = %d\n", gridX, gridY);
                }
            }
        } else if (event.type == SDL_MOUSEWHEEL) {
            float zoomSpeed = 0.1f;
            float minZoom = 2.0f;
            float maxZoom = 5.0f;

            if (event.wheel.y > 0) {
                // Zoom in
                player.zoomFactor = fminf(player.zoomFactor + zoomSpeed, maxZoom);
            } else if (event.wheel.y < 0) {
                // Zoom out
                player.zoomFactor = fmaxf(player.zoomFactor - zoomSpeed, minZoom);
            }

            printf("Zoom factor: %.2f\n", player.zoomFactor);
        }
    }
}

/*
 * UpdateGameLogic
 *
 * Updates the game logic including player and enemy states.
 */
void UpdateGameLogic() {
    UpdatePlayer(&player, allEntities, MAX_ENTITIES);

    for (int i = 0; i < MAX_ENEMIES; i++) {
        UpdateEnemy(&enemies[i], allEntities, MAX_ENTITIES);
    }
}

/*
 * Render
 *
 * Renders the game world including tiles and entities.
 */
void Render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureAtlas);
    glUniform1i(textureUniform, 0);

    float cameraOffsetX = player.cameraCurrentX;
    float cameraOffsetY = player.cameraCurrentY;
    float zoomFactor = player.zoomFactor;  // Use the player's zoom factor

    RenderTiles(cameraOffsetX, cameraOffsetY, zoomFactor);
    RenderEntities(cameraOffsetX, cameraOffsetY, zoomFactor);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    SDL_GL_SwapWindow(window);
}

/*
 * RenderTiles
 *
 * Renders the terrain tiles of the game world.
 *
 * @param[in] cameraOffsetX The horizontal camera offset
 * @param[in] cameraOffsetY The vertical camera offset
 * @param[in] zoomFactor The zoom factor applied to the view
 */
void RenderTiles(float cameraOffsetX, float cameraOffsetY, float zoomFactor) {
    glUseProgram(shaderProgram);
    glBindVertexArray(squareVAO);

    float playerWorldX = player.entity.posX;
    float playerWorldY = player.entity.posY;

    int renderedTiles = 0;
    int culledTiles = 0;

    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            float worldX, worldY;
            WorldToScreenCoords(x, y, 0, 0, 1, &worldX, &worldY);

            if (!isPointVisible(worldX, worldY, playerWorldX, playerWorldY, zoomFactor)) {
                culledTiles++;
                continue;  // Skip rendering this tile
            }

            renderedTiles++;

            float posX, posY;
            WorldToScreenCoords(x, y, cameraOffsetX, cameraOffsetY, zoomFactor, &posX, &posY);

            float texX = 0.0f;
            float texY = 0.0f;  
            float texWidth = 1.0f / 3.0f;
            float texHeight = 1.0f / 2.0f;

            switch (grid[y][x].terrainType) {
                case TERRAIN_SAND:
                    texX = 0.0f / 3.0f;
                    texY = 0.0f / 2.0f;
                    break;
                case TERRAIN_WATER:
                    texX = 1.0f / 3.0f;
                    texY = 0.0f / 2.0f;
                    break;
                case TERRAIN_GRASS:
                    texX = 2.0f / 3.0f;
                    texY = 0.0f / 2.0f;
                    break;
                default:
                    texX = 2.0f / 3.0f;
                    texY = 1.0f / 2.0f;
                    break;
            }

            float vertices[] = {
                posX - TILE_SIZE * zoomFactor, posY - TILE_SIZE * zoomFactor, texX, texY + texHeight,
                posX + TILE_SIZE * zoomFactor, posY - TILE_SIZE * zoomFactor, texX + texWidth, texY + texHeight,
                posX + TILE_SIZE * zoomFactor, posY + TILE_SIZE * zoomFactor, texX + texWidth, texY,
                posX - TILE_SIZE * zoomFactor, posY + TILE_SIZE * zoomFactor, texX, texY
            };

            glBindBuffer(GL_ARRAY_BUFFER, squareVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

            // Draw outline for player's final target tile
            if (x == player.entity.finalGoalX && y == player.entity.finalGoalY) {
                glUseProgram(outlineShaderProgram);
                glBindVertexArray(outlineVAO);

                GLint outlineColorUniform = glGetUniformLocation(outlineShaderProgram, "outlineColor");
                glUniform3f(outlineColorUniform, 1.0f, 1.0f, 0.0f); // Yellow outline

                float outlineScale = 1.05f; // Slightly larger than the tile
                float outlineVertices[] = {
                    posX - TILE_SIZE * zoomFactor * outlineScale, posY - TILE_SIZE * zoomFactor * outlineScale,
                    posX + TILE_SIZE * zoomFactor * outlineScale, posY - TILE_SIZE * zoomFactor * outlineScale,
                    posX + TILE_SIZE * zoomFactor * outlineScale, posY + TILE_SIZE * zoomFactor * outlineScale,
                    posX - TILE_SIZE * zoomFactor * outlineScale, posY + TILE_SIZE * zoomFactor * outlineScale
                };

                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(outlineVertices), outlineVertices);
                glDrawArrays(GL_LINE_LOOP, 0, 4);

                glUseProgram(shaderProgram);
                glBindVertexArray(squareVAO);
            }
        }
    }

    printf("Tiles rendered: %d, Tiles culled: %d\n", renderedTiles, culledTiles);
}

/*
 * RenderEntities
 *
 * Renders the entities (player and enemies) in the game world.
 *
 * @param[in] cameraOffsetX The horizontal camera offset
 * @param[in] cameraOffsetY The vertical camera offset
 * @param[in] zoomFactor The zoom factor applied to the view
 */
void RenderEntities(float cameraOffsetX, float cameraOffsetY, float zoomFactor) {
    glBindVertexArray(squareVAO);

    float playerWorldX = player.entity.posX;
    float playerWorldY = player.entity.posY;

    int renderedEntities = 0;
    int culledEntities = 0;

    // Render enemies
    float enemyTexX = 1.0f / 3.0f;
    float enemyTexY = 1.0 / 2.0f;
    float enemyTexWidth = 1.0f / 3.0f;
    float enemyTexHeight = 1.0f / 2.0f;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        float enemyWorldX = enemies[i].entity.posX;
        float enemyWorldY = enemies[i].entity.posY;
        
        if (!isPointVisible(enemyWorldX, enemyWorldY, playerWorldX, playerWorldY, zoomFactor)) {
            culledEntities++;
            continue;  // Skip rendering this enemy
        }

        renderedEntities++;

        float screenX = (enemyWorldX - cameraOffsetX) * zoomFactor;
        float screenY = (enemyWorldY - cameraOffsetY) * zoomFactor;

        float enemyVertices[] = {
            screenX - TILE_SIZE * zoomFactor, screenY - TILE_SIZE * zoomFactor, enemyTexX, enemyTexY,
            screenX + TILE_SIZE * zoomFactor, screenY - TILE_SIZE * zoomFactor, enemyTexX + enemyTexWidth, enemyTexY,
            screenX + TILE_SIZE * zoomFactor, screenY + TILE_SIZE * zoomFactor, enemyTexX + enemyTexWidth, enemyTexY + enemyTexHeight,
            screenX - TILE_SIZE * zoomFactor, screenY + TILE_SIZE * zoomFactor, enemyTexX, enemyTexY + enemyTexHeight
        };
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(enemyVertices), enemyVertices);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }

    // Render player (always visible)
    renderedEntities++;
    float playerTexX = 0.0f / 3.0f;
    float playerTexY = 1.0f / 2.0f;
    float playerTexWidth = 1.0f / 3.0f;
    float playerTexHeight = 1.0f / 2.0f;
    float playerVertices[] = {
        (-TILE_SIZE) * zoomFactor, (-TILE_SIZE) * zoomFactor, playerTexX, playerTexY,
        (TILE_SIZE) * zoomFactor, (-TILE_SIZE) * zoomFactor, playerTexX + playerTexWidth, playerTexY,
        (TILE_SIZE) * zoomFactor, (TILE_SIZE) * zoomFactor, playerTexX + playerTexWidth, playerTexY + playerTexHeight,
        (-TILE_SIZE) * zoomFactor, (TILE_SIZE) * zoomFactor, playerTexX, playerTexY + playerTexHeight
    };
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(playerVertices), playerVertices);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    printf("Entities rendered: %d, Entities culled: %d\n", 
           renderedEntities, culledEntities );



}

/*
 * PhysicsLoop
 *
 * Runs the physics simulation and updates entities at a fixed interval.
 *
 * @param[in] arg Argument passed to the thread (unused)
 * @return int Return value (always 0)
 */
int PhysicsLoop(void* arg) {
    (void)arg;
    while (atomic_load(&isRunning)) {
        Uint32 startTime = SDL_GetTicks();
        
        UpdateEntity(&player.entity, allEntities, MAX_ENTITIES);
        UpdatePlayer(&player, allEntities, MAX_ENTITIES);

        for (int i = 0; i < MAX_ENEMIES; i++) {
            UpdateEntity(&enemies[i].entity, allEntities, MAX_ENTITIES);
        }

        int interval = 8 - (SDL_GetTicks() - startTime);
        if (interval > 0) {
            SDL_Delay(interval);
        }
    }
    return 0;
}



/*
 * main
 *
 * Entry point for the application. Initializes the random seed and starts the game loop.
 *
 * @param[in] argc Number of command-line arguments
 * @param[in] argv Array of command-line arguments
 * @return int Exit status
 */
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    srand(time(NULL));  // Initialize random seed

    printf("Starting GameLoop...\n");
    GameLoop();
    printf("GameLoop ended.\n");

    return 0;
}
