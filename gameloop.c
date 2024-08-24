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
GLuint tilesBatchVAO;
GLuint tilesBatchVBO;
GLuint squareVAO;
GLuint squareVBO;
int vertexCount;
GLuint colorUniform;

Entity* allEntities[MAX_ENTITIES];
Player player;
Enemy enemies[MAX_ENEMIES];
atomic_int physics_load = ATOMIC_VAR_INIT(0);
Uint32 FRAME_TIME_MS = 24;
// Function declarations
void setGridSize(int size);
void generateTerrain();
void RenderEntities(float cameraOffsetX, float cameraOffsetY, float zoomFactor);
GLuint loadTexture(const char* filePath);  // New texture loading function
bool isPointVisible(float worldX, float worldY, float cameraOffsetX, float cameraOffsetY, float zoomFactor) {
    // Transform world coordinates to screen coordinates
    float screenX = (worldX - cameraOffsetX) * zoomFactor;
    float screenY = (worldY - cameraOffsetY) * zoomFactor;

    // Calculate the margin based on the tile size and zoom factor
    float margin = TILE_SIZE * 3 * zoomFactor;

    // Define the visible screen bounds in screen space (NDC) with a tile size margin
    float leftBound = -1.0f - margin;
    float rightBound = 1.0f + margin;
    float bottomBound = -1.0f - margin;
    float topBound = 1.0f + margin;
    // Check if the point (screenX, screenY) is within the expanded screen bounds
    return (screenX >= leftBound && screenX <= rightBound && screenY >= bottomBound && screenY <= topBound);
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
    Uint32 lastLogicTick = SDL_GetTicks();
    Uint32 lastRenderTick = SDL_GetTicks();

    while (atomic_load(&isRunning)) {
        Uint32 currentTick = SDL_GetTicks();

        if ((currentTick - lastLogicTick) >= GAME_LOGIC_INTERVAL_MS) {
            UpdateGameLogic();
            lastLogicTick = currentTick;
        }

        // Check physics load
        int current_load = atomic_load(&physics_load);
        if (current_load > 80) { // If physics is using more than 80% of its time
            // Increase the render interval dynamically
            FRAME_TIME_MS = 48; // Double the frame time
        } else {
            FRAME_TIME_MS = 24; // Reset to normal
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

    setGridSize(40);
    generateTerrain();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        exit(1);
    }
    printf("SDL initialized.\n");

    window = SDL_CreateWindow("2D Top-Down RPG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (window == NULL) {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        exit(1);
    }
    printf("Window created.\n");

    mainContext = SDL_GL_CreateContext(window);
    if (!mainContext) {
        fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(1);
    }
    printf("OpenGL context created.\n");

    SDL_GL_MakeCurrent(window, mainContext);
    SDL_GL_SetSwapInterval(1);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glewExperimental = GL_TRUE; 
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        fprintf(stderr, "Error initializing GLEW: %s\n", glewGetErrorString(glewError));
        SDL_GL_DeleteContext(mainContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(1);
    }
    printf("GLEW initialized.\n");

    shaderProgram = createShaderProgram();
    outlineShaderProgram = createOutlineShaderProgram();
    if (!shaderProgram || !outlineShaderProgram) {
        fprintf(stderr, "Failed to create shader programs\n");
        SDL_GL_DeleteContext(mainContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(1);
    }
    printf("Shader programs created.\n");

    colorUniform = glGetUniformLocation(shaderProgram, "color");
    textureUniform = glGetUniformLocation(shaderProgram, "textureAtlas");

    float* vertices;
    createGridVertices(&vertices, &vertexCount, 800, 800, 800 / GRID_SIZE);
    gridVAO = createGridVAO(vertices, vertexCount);
    if (!gridVAO) {
        fprintf(stderr, "Failed to create grid VAO\n");
        free(vertices);
        glDeleteProgram(shaderProgram);
        glDeleteProgram(outlineShaderProgram);
        SDL_GL_DeleteContext(mainContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(1);
    }
    free(vertices);
    printf("Grid VAO created.\n");

    glGenVertexArrays(1, &squareVAO);
    glGenBuffers(1, &squareVBO);
    glGenBuffers(1, &outlineVBO);
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

    textureAtlas = loadBMP("texture_atlas.bmp");
    if (!textureAtlas) {
        fprintf(stderr, "Failed to load texture atlas\n");
        glDeleteVertexArrays(1, &gridVAO);
        glDeleteVertexArrays(1, &squareVAO);
        glDeleteBuffers(1, &squareVBO);
        glDeleteProgram(shaderProgram);
        glDeleteProgram(outlineShaderProgram);
        SDL_GL_DeleteContext(mainContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(1);
    }

    int playerGridX, playerGridY;
    do {
        playerGridX = rand() % GRID_SIZE;
        playerGridY = rand() % GRID_SIZE;
    } while (!grid[playerGridY][playerGridX].isWalkable);

    InitPlayer(&player, playerGridX, playerGridY, MOVE_SPEED);
    printf("Player initialized at (%d, %d).\n", playerGridX, playerGridY);

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

    initializeEnemyBatchVAO();
    printf("Enemy batch VAO initialized.\n");

    initializeOutlineVAO();
    printf("Outline VAO initialized.\n");

    initializeTilesBatchVAO();
    printf("Tiles batch VAO initialized.\n");

    // Initialize GPU pathfinding
    initializeGPUPathfinding();
    printf("GPU pathfinding initialized.\n");

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
            allEntities[i] = NULL;
        }
    }
}
void drawTargetTileOutline(int x, int y, float cameraOffsetX, float cameraOffsetY, float zoomFactor) {
    glUseProgram(outlineShaderProgram);
    glBindVertexArray(outlineVAO);

    GLint outlineColorUniform = glGetUniformLocation(outlineShaderProgram, "outlineColor");
    glUniform3f(outlineColorUniform, 1.0f, 1.0f, 0.0f); // Yellow outline

    float posX, posY;
    WorldToScreenCoords(x, y, cameraOffsetX, cameraOffsetY, zoomFactor, &posX, &posY);

    float outlineScale = 1.05f; // Slightly larger than the tile
    float halfSize = TILE_SIZE * zoomFactor * outlineScale;
    
    float outlineVertices[] = {
        posX - halfSize, posY - halfSize,
        posX + halfSize, posY - halfSize,
        posX + halfSize, posY + halfSize,
        posX - halfSize, posY + halfSize
    };

    glBindBuffer(GL_ARRAY_BUFFER, outlineVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(outlineVertices), outlineVertices);
    glDrawArrays(GL_LINE_LOOP, 0, 4);

    glUseProgram(shaderProgram);
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
        textureAtlas = 0;
    }
    if (enemyBatchVAO) {
        glDeleteVertexArrays(1, &enemyBatchVAO);
    }
    if (enemyBatchVBO) {
        glDeleteBuffers(1, &enemyBatchVBO);
    }
    if (tilesBatchVAO) {
        glDeleteVertexArrays(1, &tilesBatchVAO);
    }
    if (tilesBatchVBO) {
        glDeleteBuffers(1, &tilesBatchVBO);
    }
    if (mainContext) {
        SDL_GL_DeleteContext(mainContext);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    cleanupGrid();

    // Clean up GPU pathfinding resources
    cleanupGPUPathfinding();

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

    Uint32 currentTime = SDL_GetTicks(); // Get the current time

    for (int i = 0; i < MAX_ENEMIES; i++) {
        UpdateEnemy(&enemies[i], allEntities, MAX_ENTITIES, currentTime);
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
    float zoomFactor = player.zoomFactor;

    RenderTiles(cameraOffsetX, cameraOffsetY, zoomFactor);
    RenderEntities(cameraOffsetX, cameraOffsetY, zoomFactor);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    SDL_GL_SwapWindow(window);
}
#define MAX_VISIBLE_TILES (GRID_SIZE * GRID_SIZE)
void initializeTilesBatchVAO() {
    glGenVertexArrays(1, &tilesBatchVAO);
    glGenBuffers(1, &tilesBatchVBO);

    glBindVertexArray(tilesBatchVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tilesBatchVBO);

    glBufferData(GL_ARRAY_BUFFER, MAX_VISIBLE_TILES * 6 * 4 * sizeof(float), NULL, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
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
#define MAX_VISIBLE_TILES (GRID_SIZE * GRID_SIZE)

void RenderTiles(float cameraOffsetX, float cameraOffsetY, float zoomFactor) {
    glUseProgram(shaderProgram);
    glBindVertexArray(tilesBatchVAO);

    float playerWorldX = player.entity.posX;
    float playerWorldY = player.entity.posY;

    int renderedTiles = 0;
    int culledTiles = 0;

    float* batchData = (float*)malloc(MAX_VISIBLE_TILES * 6 * 4 * sizeof(float));
    if (!batchData) {
        fprintf(stderr, "Failed to allocate memory for tile batch data\n");
        return;
    }

    int dataIndex = 0;

    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            float worldX, worldY;
            WorldToScreenCoords(x, y, 0, 0, 1, &worldX, &worldY);

            if (!isPointVisible(worldX, worldY, playerWorldX, playerWorldY, zoomFactor)) {
                culledTiles++;
                continue;  // Skip this tile
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

            float halfSize = TILE_SIZE * zoomFactor;

            // First triangle
            batchData[dataIndex++] = posX - halfSize;
            batchData[dataIndex++] = posY - halfSize;
            batchData[dataIndex++] = texX;
            batchData[dataIndex++] = texY + texHeight;

            batchData[dataIndex++] = posX + halfSize;
            batchData[dataIndex++] = posY - halfSize;
            batchData[dataIndex++] = texX + texWidth;
            batchData[dataIndex++] = texY + texHeight;

            batchData[dataIndex++] = posX - halfSize;
            batchData[dataIndex++] = posY + halfSize;
            batchData[dataIndex++] = texX;
            batchData[dataIndex++] = texY;

            // Second triangle
            batchData[dataIndex++] = posX + halfSize;
            batchData[dataIndex++] = posY - halfSize;
            batchData[dataIndex++] = texX + texWidth;
            batchData[dataIndex++] = texY + texHeight;

            batchData[dataIndex++] = posX + halfSize;
            batchData[dataIndex++] = posY + halfSize;
            batchData[dataIndex++] = texX + texWidth;
            batchData[dataIndex++] = texY;

            batchData[dataIndex++] = posX - halfSize;
            batchData[dataIndex++] = posY + halfSize;
            batchData[dataIndex++] = texX;
            batchData[dataIndex++] = texY;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, tilesBatchVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, dataIndex * sizeof(float), batchData);
    glDrawArrays(GL_TRIANGLES, 0, renderedTiles * 6);

    free(batchData);

    // Draw outline for player's final target tile
    if (isPointVisible(player.entity.finalGoalX, player.entity.finalGoalY, playerWorldX, playerWorldY, zoomFactor)) {
        drawTargetTileOutline(player.entity.finalGoalX, player.entity.finalGoalY, cameraOffsetX, cameraOffsetY, zoomFactor);
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
    float playerWorldX = player.entity.posX;
    float playerWorldY = player.entity.posY;

    int visibleEnemyCount = 0;
    int culledEnemyCount = 0;

    // Allocate memory for only visible enemies
    Enemy visibleEnemies[MAX_ENEMIES];

    // Frustum culling to count visible enemies
   for (int i = 0; i < MAX_ENEMIES; i++) {
        float enemyWorldX = enemies[i].entity.posX;
        float enemyWorldY = enemies[i].entity.posY;

        if (isPointVisible(enemyWorldX, enemyWorldY, playerWorldX, playerWorldY, zoomFactor)) {
            visibleEnemies[visibleEnemyCount++] = enemies[i];
        } else {
            culledEnemyCount++;
        }
    }

    // Log summary of visible and culled enemies
    printf("Total enemies: %d, Visible enemies: %d, Culled enemies: %d\n", MAX_ENEMIES, visibleEnemyCount, culledEnemyCount);



    // Use the smooth camera values from the player
    float smoothCameraOffsetX = player.cameraCurrentX;
    float smoothCameraOffsetY = player.cameraCurrentY;

    // Render the enemy batch
    updateEnemyBatchVBO(visibleEnemies, visibleEnemyCount, cameraOffsetX, cameraOffsetY, zoomFactor);
    glBindVertexArray(enemyBatchVAO);
    glDrawArrays(GL_TRIANGLES, 0, visibleEnemyCount * 6);  // Draw only the visible enemies

    // Render player (unchanged)
    glBindVertexArray(squareVAO);
    glBindBuffer(GL_ARRAY_BUFFER, squareVBO);

    float playerScreenX = (playerWorldX - smoothCameraOffsetX) * zoomFactor;
    float playerScreenY = (playerWorldY - smoothCameraOffsetY) * zoomFactor;
    float playerTexX = 0.0f / 3.0f;
    float playerTexY = 1.0f / 2.0f;
    float playerTexWidth = 1.0f / 3.0f;
    float playerTexHeight = 1.0f / 2.0f;
    float playerVertices[] = {
        playerScreenX - TILE_SIZE * zoomFactor, playerScreenY - TILE_SIZE * zoomFactor, playerTexX, playerTexY,
        playerScreenX + TILE_SIZE * zoomFactor, playerScreenY - TILE_SIZE * zoomFactor, playerTexX + playerTexWidth, playerTexY,
        playerScreenX + TILE_SIZE * zoomFactor, playerScreenY + TILE_SIZE * zoomFactor, playerTexX + playerTexWidth, playerTexY + playerTexHeight,
        playerScreenX - TILE_SIZE * zoomFactor, playerScreenY + TILE_SIZE * zoomFactor, playerTexX, playerTexY + playerTexHeight
    };
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(playerVertices), playerVertices);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
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
        
        atomic_store(&physics_load, 100); // Assume 100% load at start

        UpdateEntity(&player.entity, allEntities, MAX_ENTITIES);
        UpdatePlayer(&player, allEntities, MAX_ENTITIES);

        for (int i = 0; i < MAX_ENEMIES; i++) {
            UpdateEntity(&enemies[i].entity, allEntities, MAX_ENTITIES);
        }

        Uint32 endTime = SDL_GetTicks();
        Uint32 elapsedTime = endTime - startTime;
        
        // Calculate actual load percentage
        int load = (elapsedTime * 100) / 8; // Assuming 8ms target time
        atomic_store(&physics_load, load);

        if (elapsedTime < 8) {
            SDL_Delay(8 - elapsedTime);
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

    srand((unsigned int)time(NULL));  

    printf("Starting GameLoop...\n");
    GameLoop();
    printf("GameLoop ended.\n");

    return 0;
}
