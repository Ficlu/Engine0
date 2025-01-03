// gameloop.c

#include "gameloop.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <immintrin.h>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#include <stdatomic.h>
#include <time.h>
#include "rendering.h"
#include "asciiMap.h"
#include "player.h"
#include "enemy.h"
#include "grid.h"
#include "entity.h"
#include "pathfinding.h"
#include "saveload.h"

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
bool placementModeActive = false;
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

    // Set grid size (still uses a fixed 40 in your code)
    setGridSize(40);

    // -----------------------------------------
    // 1) Initialize the chunk manager *first*,
    //    but DO NOT load chunks yet
    // -----------------------------------------
    globalChunkManager = (ChunkManager*)malloc(sizeof(ChunkManager));
    if (!globalChunkManager) {
        fprintf(stderr, "Failed to allocate chunk manager\n");
        exit(1);
    }
    initChunkManager(globalChunkManager, 1); // e.g. loadRadius=2
    printf("Chunk manager initialized.\n");

    // -----------------------------------------
    // 2) Initialize the global grid to some defaults
    // -----------------------------------------
    initializeGrid(GRID_SIZE);
    printf("Grid initialized.\n");

    // -----------------------------------------
    // 3) Load ASCII map from file
    //
    //    -- We have removed the code from
    //       loadASCIIMap() that did chunk loading.
    // -----------------------------------------
    char* asciiMap = loadASCIIMap("testmap.txt");
    if (!asciiMap) {
        fprintf(stderr, "Failed to load test map!\n");
        exit(1);
    }
    printf("ASCII map loaded successfully.\n");

    // -----------------------------------------
    // 4) Initialize SDL, create Window, GL Context, etc.
    // -----------------------------------------
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        exit(1);
    }
    printf("SDL initialized.\n");

    window = SDL_CreateWindow("2D Top-Down RPG",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, 
                              WINDOW_WIDTH, WINDOW_HEIGHT,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
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

    // Create our two shader programs (main & outline)
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

    // Get uniform locations for color and texture
    colorUniform = glGetUniformLocation(shaderProgram, "color");
    textureUniform = glGetUniformLocation(shaderProgram, "textureAtlas");

    // -----------------------------------------
    // 5) Create grid VAO
    // -----------------------------------------
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

    // -----------------------------------------
    // 6) Create Square VAO & VBO (for player, etc.)
    // -----------------------------------------
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

    // -----------------------------------------
    // 7) Load texture atlas
    // -----------------------------------------
    textureAtlas = loadBMP("texture_atlas-1.bmp");
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

    // -----------------------------------------
    // 8) Initialize the player in the center
    // -----------------------------------------
    int playerGridX = GRID_SIZE / 2;
    int playerGridY = GRID_SIZE / 2;
    InitPlayer(&player, playerGridX, playerGridY, MOVE_SPEED);
    printf("Player initialized at (%d, %d).\n", playerGridX, playerGridY);

    // -----------------------------------------
    // 9) Now that the player is placed, do the
    //    initial chunk load *once* here
    // -----------------------------------------
    ChunkCoord playerStartChunk = getChunkFromWorldPos(playerGridX, playerGridY);
    globalChunkManager->playerChunk = playerStartChunk;
    loadChunksAroundPlayer(globalChunkManager);
    printf("Initial chunks loaded around player.\n");

    // -----------------------------------------
    // 10) Initialize all enemies
    // -----------------------------------------
    allEntities[0] = &player.entity;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        int enemyGridX, enemyGridY;
        do {
            enemyGridX = rand() % GRID_SIZE;
            enemyGridY = rand() % GRID_SIZE;
        } while (!grid[enemyGridY][enemyGridX].isWalkable ||
                 (enemyGridX == playerGridX && enemyGridY == playerGridY));

        InitEnemy(&enemies[i], enemyGridX, enemyGridY, MOVE_SPEED);
        allEntities[i + 1] = &enemies[i].entity;
        printf("Enemy %d initialized at (%d, %d).\n", i, enemyGridX, enemyGridY);
    }

    // -----------------------------------------
    // 11) Initialize VAOs for enemies, outlines, and tiles
    // -----------------------------------------
    initializeEnemyBatchVAO();
    printf("Enemy batch VAO initialized.\n");

    initializeOutlineVAO();
    printf("Outline VAO initialized.\n");

    initializeTilesBatchVAO();
    printf("Tiles batch VAO initialized.\n");

    // -----------------------------------------
    // 12) Initialize GPU-based pathfinding
    // -----------------------------------------
    initializeGPUPathfinding();
    printf("GPU pathfinding initialized.\n");

    // -----------------------------------------
    // 13) Apply initial chunk culling
    // -----------------------------------------
    ChunkCoord playerChunk = getChunkFromWorldPos(player.entity.gridX, player.entity.gridY);
    int radius = globalChunkManager->loadRadius;
    
    // Cull everything outside the initial radius
    for (int cy = 0; cy < NUM_CHUNKS; cy++) {
        for (int cx = 0; cx < NUM_CHUNKS; cx++) {
            int dx = abs(cx - playerChunk.x);
            int dy = abs(cy - playerChunk.y);
            
            // If chunk is outside radius, mark all its cells as unloaded
            if (dx > radius || dy > radius) {
                int startX = cx * CHUNK_SIZE;
                int startY = cy * CHUNK_SIZE;
                
                for (int y = 0; y < CHUNK_SIZE; y++) {
                    for (int x = 0; x < CHUNK_SIZE; x++) {
                        int gridX = startX + x;
                        int gridY = startY + y;
                        if (gridX >= 0 && gridX < GRID_SIZE && 
                            gridY >= 0 && gridY < GRID_SIZE) {
                            grid[gridY][gridX].terrainType = TERRAIN_UNLOADED;
                            grid[gridY][gridX].isWalkable = false;
                        }
                    }
                }
            }
        }
    }
    
    printf("Initial chunk culling complete.\n");
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
    if (globalChunkManager) {
        cleanupChunkManager(globalChunkManager);
        free(globalChunkManager);
        globalChunkManager = NULL;
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

    cleanupEntityBatchData();
    cleanupTileBatchData();

    if (mainContext) {
        SDL_GL_DeleteContext(mainContext);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    cleanupGrid();

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
        } 
        else if (event.type == SDL_KEYDOWN) {
            printf("Key pressed: %d\n", event.key.keysym.sym);
            if (event.key.keysym.sym == SDLK_e) {
                placementModeActive = !placementModeActive;
                printf("Placement mode %s\n", placementModeActive ? "activated" : "deactivated");
            }
                        // Add save/load shortcuts
            else if (event.key.keysym.sym == SDLK_s) {
                if (saveGameState("game_save.sav")) {
                    printf("Game saved successfully!\n");
                } else {
                    printf("Failed to save game!\n");
                }
            }
            else if (event.key.keysym.sym == SDLK_l) {
                if (loadGameState("game_save.sav")) {
                    printf("Game loaded successfully!\n");
                } else {
                    printf("Failed to load game!\n");
                }
            }
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN) {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);

            float ndcX = (2.0f * mouseX / WINDOW_WIDTH - 1.0f) / player.zoomFactor;
            float ndcY = (1.0f - 2.0f * mouseY / WINDOW_HEIGHT) / player.zoomFactor;

            float worldX = ndcX + player.cameraCurrentX;
            float worldY = ndcY + player.cameraCurrentY;

            int gridX = (int)((worldX + 1.0f) * GRID_SIZE / 2);
            int gridY = (int)((1.0f - worldY) * GRID_SIZE / 2);

            if (gridX >= 0 && gridX < GRID_SIZE && gridY >= 0 && gridY < GRID_SIZE) {
                if (placementModeActive) {
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        int playerGridX = player.entity.gridX;
                        int playerGridY = player.entity.gridY;
                        bool isNearby = (
                            abs(gridX - playerGridX) <= 1 && 
                            abs(gridY - playerGridY) <= 1
                        );

                        bool isTileOccupied = (gridX == playerGridX && gridY == playerGridY);
                        
                        for (int i = 0; i < MAX_ENTITIES; i++) {
                            if (allEntities[i] != NULL) {
                                if (atomic_load(&allEntities[i]->gridX) == gridX && 
                                    atomic_load(&allEntities[i]->gridY) == gridY) {
                                    isTileOccupied = true;
                                    break;
                                }
                            }
                        }

                        if (isNearby && !isTileOccupied) {
                            grid[gridY][gridX].hasWall = true;
                            grid[gridY][gridX].isWalkable = false;

                            // Check surrounding walls and update textures
                            bool hasNorth = (gridY > 0) && grid[gridY-1][gridX].hasWall;
                            bool hasSouth = (gridY < GRID_SIZE-1) && grid[gridY+1][gridX].hasWall;
                            bool hasEast = (gridX < GRID_SIZE-1) && grid[gridY][gridX+1].hasWall;
                            bool hasWest = (gridX > 0) && grid[gridY][gridX-1].hasWall;

                            // Vertical wall check (has connection north OR south)
                            bool isVertical = hasNorth || hasSouth;

                            // Set texture coordinates based on surrounding walls
                            if (hasSouth && hasEast) {
                                grid[gridY][gridX].wallTexX = 1.0f / 3.0f;
                                grid[gridY][gridX].wallTexY = 1.0f / 5.0f;  // Top-left corner
                            }
                            else if (hasSouth && hasWest) {
                                grid[gridY][gridX].wallTexX = 2.0f / 3.0f;
                                grid[gridY][gridX].wallTexY = 2.0f / 5.0f;  // Top-right corner
                            }
                            else if (hasNorth && hasEast) {
                                grid[gridY][gridX].wallTexX = 0.0f / 3.0f;
                                grid[gridY][gridX].wallTexY = 1.0f / 5.0f;  // Bottom-left corner
                            }
                            else if (hasNorth && hasWest) {
                                grid[gridY][gridX].wallTexX = 2.0f / 3.0f;
                                grid[gridY][gridX].wallTexY = 1.0f / 5.0f;  // Bottom-right corner
                            }
                            else if (isVertical) {
                                grid[gridY][gridX].wallTexX = 0.0f / 3.0f;
                                grid[gridY][gridX].wallTexY = 2.0f / 5.0f;  // Vertical wall
                            }
                            else {
                                grid[gridY][gridX].wallTexX = 1.0f / 3.0f;
                                grid[gridY][gridX].wallTexY = 2.0f / 5.0f;  // Front wall (default)
                            }

                            printf("Placed wall at grid position: %d, %d\n", gridX, gridY);
                        } else if (isTileOccupied) {
                            printf("Can't place wall - tile is occupied\n");
                        } else {
                            if (grid[gridY][gridX].isWalkable) {
                                player.entity.finalGoalX = gridX;
                                player.entity.finalGoalY = gridY;
                                player.entity.targetGridX = player.entity.gridX;
                                player.entity.targetGridY = player.entity.gridY;
                                player.entity.needsPathfinding = true;
                                player.targetBuildX = gridX;
                                player.targetBuildY = gridY;
                                player.hasBuildTarget = true;
                                printf("Moving to wall placement location: %d, %d\n", gridX, gridY);
                            } else {
                                printf("Can't move to that location - tile is not walkable\n");
                            }
                        }
                    }
else if (event.button.button == SDL_BUTTON_RIGHT) {
    // Check if there's a wall to remove
    if (grid[gridY][gridX].hasWall) {
        int playerGridX = player.entity.gridX;
        int playerGridY = player.entity.gridY;
        bool isNearby = (
            abs(gridX - playerGridX) <= 1 && 
            abs(gridY - playerGridY) <= 1
        );

        if (isNearby) {
            // Simply remove the wall
            grid[gridY][gridX].hasWall = false;
            grid[gridY][gridX].isWalkable = true;
            printf("Removed wall at grid position: %d, %d\n", gridX, gridY);
        } else {
            printf("Can't remove wall - too far away\n");
        }
    }
}
                }
                else if (grid[gridY][gridX].isWalkable) {
                    player.entity.finalGoalX = gridX;
                    player.entity.finalGoalY = gridY;
                    player.entity.targetGridX = player.entity.gridX;
                    player.entity.targetGridY = player.entity.gridY;
                    player.entity.needsPathfinding = true;
                    printf("Player final goal set: gridX = %d, gridY = %d\n", gridX, gridY);
                }
            }
        } else if (event.type == SDL_MOUSEWHEEL) {
            float zoomSpeed = 0.2f;
            float minZoom = 0.2f;
            float maxZoom = 20.0f;

            if (event.wheel.y > 0) {
                player.zoomFactor = fminf(player.zoomFactor + zoomSpeed, maxZoom);
            } else if (event.wheel.y < 0) {
                player.zoomFactor = fmaxf(player.zoomFactor - zoomSpeed, minZoom);
            }

            printf("Zoom factor: %.2f\n", player.zoomFactor);
        }
    }
}
bool westIsCorner(int x, int y) {
    if (x <= 0) return false;
    if (!grid[y][x-1].hasWall) return false;
    
    float texY = grid[y][x-1].wallTexY;
    return (texY == 0.0f/4.0f) ||    // Top corners row
           (texY == 1.0f/4.0f);      // Bottom corners row
}

bool eastIsCorner(int x, int y) {
    if (x >= GRID_SIZE-1) return false; 
    if (!grid[y][x+1].hasWall) return false;

    float texY = grid[y][x+1].wallTexY;
    return (texY == 0.0f/4.0f) ||    // Top corners row
           (texY == 1.0f/4.0f);      // Bottom corners row
}

/*
 * UpdateGameLogic
 *
 * Updates the game logic including player and enemy states.
 */
void UpdateGameLogic() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (isPositionInLoadedChunk(enemies[i].entity.posX, enemies[i].entity.posY)) {
            UpdateEnemy(&enemies[i], allEntities, MAX_ENTITIES, SDL_GetTicks());
        }
        // Entities in unloaded chunks retain their state but aren't updated
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

void RenderTiles(float cameraOffsetX, float cameraOffsetY, float zoomFactor) {
    if (!tileBatchData.persistentBuffer) {
        tileBatchData.bufferCapacity = MAX_VISIBLE_TILES * 6 * 4;
        tileBatchData.persistentBuffer = malloc(tileBatchData.bufferCapacity * sizeof(float));
        if (!tileBatchData.persistentBuffer) {
            fprintf(stderr, "Failed to allocate tile batch buffer\n");
            return;
        }
    }

    glUseProgram(shaderProgram);
    glBindVertexArray(tilesBatchVAO);

    float playerWorldX = player.entity.posX;
    float playerWorldY = player.entity.posY;

    int renderedTiles = 0;
    int culledTiles = 0;

    int dataIndex = 0;
    float* batchData = tileBatchData.persistentBuffer;

    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (grid[y][x].terrainType == TERRAIN_UNLOADED) {
                continue;
            }

            float worldX, worldY;
            WorldToScreenCoords(x, y, 0, 0, 1, &worldX, &worldY);

            if (!isPointVisible(worldX, worldY, playerWorldX, playerWorldY, zoomFactor)) {
                culledTiles++;
                continue;
            }

            renderedTiles++;

            float posX, posY;
            WorldToScreenCoords(x, y, cameraOffsetX, cameraOffsetY, zoomFactor, &posX, &posY);

            float texX = 0.0f;
            float texY = 0.0f;
            float texWidth = 1.0f / 3.0f;
            float texHeight = 1.0f / 5.0f;

            switch (grid[y][x].terrainType) {
                case TERRAIN_SAND:
                    texX = 0.0f / 3.0f;
                    texY = 3.0f / 5.0f;
                    break;
                case TERRAIN_WATER:
                    texX = 1.0f / 3.0f;
                    texY = 3.0f / 5.0f;
                    break;
                case TERRAIN_GRASS:
                    texX = 2.0f / 3.0f;
                    texY = 3.0f / 5.0f;
                    break;
                default:
                    texX = 2.0f / 3.0f;
                    texY = 3.0f / 5.0f;
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

            if (grid[y][x].hasWall) {
                float wallTexX = grid[y][x].wallTexX;
                float wallTexY = grid[y][x].wallTexY;

                // First triangle for wall
                batchData[dataIndex++] = posX - halfSize;
                batchData[dataIndex++] = posY - halfSize;
                batchData[dataIndex++] = wallTexX;
                batchData[dataIndex++] = wallTexY;

                batchData[dataIndex++] = posX + halfSize;
                batchData[dataIndex++] = posY - halfSize;
                batchData[dataIndex++] = wallTexX + texWidth;
                batchData[dataIndex++] = wallTexY;

                batchData[dataIndex++] = posX - halfSize;
                batchData[dataIndex++] = posY + halfSize;
                batchData[dataIndex++] = wallTexX;
                batchData[dataIndex++] = wallTexY + texHeight;

                // Second triangle for wall
                batchData[dataIndex++] = posX + halfSize;
                batchData[dataIndex++] = posY - halfSize;
                batchData[dataIndex++] = wallTexX + texWidth;
                batchData[dataIndex++] = wallTexY;

                batchData[dataIndex++] = posX + halfSize;
                batchData[dataIndex++] = posY + halfSize;
                batchData[dataIndex++] = wallTexX + texWidth;
                batchData[dataIndex++] = wallTexY + texHeight;

                batchData[dataIndex++] = posX - halfSize;
                batchData[dataIndex++] = posY + halfSize;
                batchData[dataIndex++] = wallTexX;
                batchData[dataIndex++] = wallTexY + texHeight;

                renderedTiles++;
            }
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, tilesBatchVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, dataIndex * sizeof(float), batchData);
    glDrawArrays(GL_TRIANGLES, 0, renderedTiles * 6);

    if (isPointVisible(player.entity.finalGoalX, player.entity.finalGoalY, playerWorldX, playerWorldY, zoomFactor)) {
        drawTargetTileOutline(player.entity.finalGoalX, player.entity.finalGoalY, cameraOffsetX, cameraOffsetY, zoomFactor);
    }

    //printf("Tiles rendered: %d, Tiles culled: %d\n", renderedTiles, culledTiles);
}

void updateWallTextures(int x, int y) {
    // Check all adjacent cells
    bool hasNorth = (y > 0) && grid[y-1][x].hasWall;
    bool hasSouth = (y < GRID_SIZE-1) && grid[y+1][x].hasWall;
    bool hasEast = (x < GRID_SIZE-1) && grid[y][x+1].hasWall;
    bool hasWest = (x > 0) && grid[y][x-1].hasWall;

    // Update current cell
    if (grid[y][x].hasWall) {
        WallTextureCoords coords;

        // Corner cases
        if (hasNorth && hasEast && !hasWest && !hasSouth) {
            coords.texX = WALL_BOTTOM_LEFT_TEX_X;
            coords.texY = WALL_BOTTOM_LEFT_TEX_Y;
        }
        else if (hasNorth && hasWest && !hasEast && !hasSouth) {
            coords.texX = WALL_BOTTOM_RIGHT_TEX_X;
            coords.texY = WALL_BOTTOM_RIGHT_TEX_Y;
        }
        else if (hasSouth && hasEast && !hasWest && !hasNorth) {
            coords.texX = WALL_TOP_LEFT_TEX_X;
            coords.texY = WALL_TOP_LEFT_TEX_Y;
        }
        else if (hasSouth && hasWest && !hasEast && !hasNorth) {
            coords.texX = WALL_TOP_RIGHT_TEX_X;
            coords.texY = WALL_TOP_RIGHT_TEX_Y;
        }
        // Vertical wall (default)
        else if (hasNorth || hasSouth) {
            coords.texX = WALL_VERTICAL_TEX_X;
            coords.texY = WALL_VERTICAL_TEX_Y;
        }
        // Front-facing wall
        else {
            coords.texX = WALL_FRONT_TEX_X;
            coords.texY = WALL_FRONT_TEX_Y;
        }

        // Store texture coordinates in grid cell
        grid[y][x].wallTexX = coords.texX;
        grid[y][x].wallTexY = coords.texY;
    }

    // Update adjacent walls
    if (hasNorth) updateWallTextures(x, y-1);
    if (hasSouth) updateWallTextures(x, y+1);
    if (hasEast) updateWallTextures(x+1, y);
    if (hasWest) updateWallTextures(x-1, y);
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
    int chunkCulledCount = 0;

    Enemy visibleEnemies[MAX_ENEMIES];

    __m128 zoomFactorVec = _mm_set1_ps(zoomFactor);
    __m128 marginVec = _mm_set1_ps(TILE_SIZE);
    __m128 minusOne = _mm_set1_ps(-1.0f);
    __m128 one = _mm_set1_ps(1.0f);

    __m128 leftBound = _mm_sub_ps(minusOne, marginVec);
    __m128 rightBound = _mm_add_ps(one, marginVec);
    __m128 bottomBound = _mm_sub_ps(minusOne, marginVec);
    __m128 topBound = _mm_add_ps(one, marginVec);

    for (int i = 0; i < MAX_ENEMIES; i += 4) {
        bool enemyValid[4] = {false, false, false, false};
        int validEnemiesInGroup = (MAX_ENEMIES - i) < 4 ? (MAX_ENEMIES - i) : 4;

        // First check which enemies are in loaded chunks
        for (int j = 0; j < validEnemiesInGroup; j++) {
            if (isPositionInLoadedChunk(enemies[i+j].entity.posX, enemies[i+j].entity.posY)) {
                enemyValid[j] = true;
            } else {
                chunkCulledCount++;
            }
        }

        __m128 enemyPosX = _mm_set_ps(
            enemies[i+3].entity.posX, enemies[i+2].entity.posX,
            enemies[i+1].entity.posX, enemies[i].entity.posX
        );
        __m128 enemyPosY = _mm_set_ps(
            enemies[i+3].entity.posY, enemies[i+2].entity.posY,
            enemies[i+1].entity.posY, enemies[i].entity.posY
        );

        __m128 screenX = _mm_mul_ps(_mm_sub_ps(enemyPosX, _mm_set1_ps(playerWorldX)), zoomFactorVec);
        __m128 screenY = _mm_mul_ps(_mm_sub_ps(enemyPosY, _mm_set1_ps(playerWorldY)), zoomFactorVec);

        __m128 visibleX = _mm_and_ps(_mm_cmpge_ps(screenX, leftBound), _mm_cmple_ps(screenX, rightBound));
        __m128 visibleY = _mm_and_ps(_mm_cmpge_ps(screenY, bottomBound), _mm_cmple_ps(screenY, topBound));
        __m128 visible = _mm_and_ps(visibleX, visibleY);

        int visibilityMask = _mm_movemask_ps(visible);

        for (int j = 0; j < validEnemiesInGroup; ++j) {
            if (enemyValid[j] && (visibilityMask & (1 << j))) {
                visibleEnemies[visibleEnemyCount++] = enemies[i + j];
            } else if (enemyValid[j]) {
                culledEnemyCount++;
            }
        }
    }

    //printf("Total enemies: %d, Visible: %d, View-culled: %d, Chunk-culled: %d\n", 
    //       MAX_ENEMIES, visibleEnemyCount, culledEnemyCount, chunkCulledCount);

    float smoothCameraOffsetX = player.cameraCurrentX;
    float smoothCameraOffsetY = player.cameraCurrentY;

    updateEnemyBatchVBO(visibleEnemies, visibleEnemyCount, smoothCameraOffsetX, smoothCameraOffsetY, zoomFactor);
    glBindVertexArray(enemyBatchVAO);
    glDrawArrays(GL_TRIANGLES, 0, visibleEnemyCount * 6);

    // Render player
    glBindVertexArray(squareVAO);
    glBindBuffer(GL_ARRAY_BUFFER, squareVBO);

    float playerScreenX = (playerWorldX - smoothCameraOffsetX) * zoomFactor;
    float playerScreenY = (playerWorldY - smoothCameraOffsetY) * zoomFactor;
    float playerTexX = 0.0f / 3.0f;
    float playerTexY = 4.0f / 5.0f;
    float playerTexWidth = 1.0f / 3.0f;
    float playerTexHeight = 1.0f / 5.0f;

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
        
        atomic_store(&physics_load, 100);

        // Update player physics
        UpdateEntity(&player.entity, allEntities, MAX_ENTITIES);
        UpdatePlayer(&player, allEntities, MAX_ENTITIES);
        
        // Check for chunk updates immediately after player moves
        if (globalChunkManager) {
            updatePlayerChunk(globalChunkManager, player.entity.posX, player.entity.posY);
        }

        // Update other entities - but only if they're in loaded chunks
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (isPositionInLoadedChunk(enemies[i].entity.posX, enemies[i].entity.posY)) {
                UpdateEntity(&enemies[i].entity, allEntities, MAX_ENTITIES);
            }
            // Entities in unloaded chunks retain their state but aren't updated
        }

        Uint32 endTime = SDL_GetTicks();
        Uint32 elapsedTime = endTime - startTime;
        
        int load = (elapsedTime * 100) / 8;
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
