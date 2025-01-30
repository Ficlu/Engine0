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
#include "structures.h"
#include "input.h"
#include "ui.h"
#include "texture_coords.h"
#include "storage.h"
#include "overlay.h"
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
atomic_uint game_ticks = ATOMIC_VAR_INIT(0);  
// Function declarations
void setGridSize(int size);
void generateTerrain();
void RenderEntities(float cameraOffsetX, float cameraOffsetY, float zoomFactor);
GLuint loadTexture(const char* filePath);  // New texture loading function
StructureType currentStructureType = STRUCTURE_WALL;
PlacementMode placementMode = {
    .active = false,
    .currentType = STRUCTURE_WALL,
    .previewX = 0,
    .previewY = 0,
    .opacity = 0.5f,
    .validPlacement = false
};
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
void Initialize(void) {
    InitializeEngine();
    InitializeGameState(true);  // true = new game
}

void InitializeGameState(bool isNewGame) {
   printf("Initializing game state...\n");

   setGridSize(40);

   globalChunkManager = (ChunkManager*)malloc(sizeof(ChunkManager));
   if (!globalChunkManager) {
       fprintf(stderr, "Failed to allocate chunk manager\n");
       exit(1);
   }
   initChunkManager(globalChunkManager, 1); // chunk radius
   printf("Chunk manager initialized.\n");

   initEnclosureManager(&globalEnclosureManager);
   printf("Enclosure manager initialized.\n");

    initStorageManager(&globalStorageManager);
    printf("Storage manager initialized.\n");
   if (isNewGame) {
       // Load and apply ASCII map first
       char* asciiMap = loadASCIIMap("testmap.txt");
       if (!asciiMap) {
           fprintf(stderr, "Failed to load test map!\n");
           exit(1);
       }
       printf("ASCII map loaded successfully.\n");

       // Generate terrain from ASCII map
       generateTerrainFromASCII(asciiMap);
       printf("Terrain generated from ASCII map.\n");

       // First initialize base grid properties 
       for (int y = 0; y < GRID_SIZE; y++) {
           for (int x = 0; x < GRID_SIZE; x++) {
               // Don't touch terrainType, it's already set
               grid[y][x].structureType = 0;
               grid[y][x].materialType = 0;
               grid[y][x].biomeType = BIOME_PLAINS;  // This should probably be based on terrain type
               GRIDCELL_SET_WALKABLE(grid[y][x], grid[y][x].terrainType != TERRAIN_WATER);
               GRIDCELL_SET_ORIENTATION(grid[y][x], 0);
               grid[y][x].wallTexX = 0.0f;
               grid[y][x].wallTexY = 0.0f;
           }
       }

       // Initialize entities
       int playerGridX = GRID_SIZE / 2;
       int playerGridY = GRID_SIZE / 2;
       InitPlayer(&player, playerGridX, playerGridY, MOVE_SPEED);
       printf("Player initialized at (%d, %d).\n", playerGridX, playerGridY);

       // First store all current terrain data
       for (int cy = 0; cy < NUM_CHUNKS; cy++) {
           for (int cx = 0; cx < NUM_CHUNKS; cx++) {
               int startX = cx * CHUNK_SIZE;
               int startY = cy * CHUNK_SIZE;
               
               for (int y = 0; y < CHUNK_SIZE; y++) {
                   for (int x = 0; x < CHUNK_SIZE; x++) {
                       int gridX = startX + x;
                       int gridY = startY + y;
                       if (gridX >= 0 && gridX < GRID_SIZE && 
                           gridY >= 0 && gridY < GRID_SIZE) {
                           globalChunkManager->storedChunkData[cy][cx][y][x] = grid[gridY][gridX];
                       }
                   }
               }
               globalChunkManager->chunkHasData[cy][cx] = true;
           }
       }

       // THEN initialize and load chunks after base grid is set up
       ChunkCoord playerStartChunk = getChunkFromWorldPos(player.entity.gridX, player.entity.gridY);
       globalChunkManager->playerChunk = playerStartChunk;
       loadChunksAroundPlayer(globalChunkManager);
       printf("Initial chunks loaded around player.\n");
   }

   // Initialize entity array first
   allEntities[0] = &player.entity;
   for(int i = 1; i < MAX_ENTITIES; i++) {
       allEntities[i] = NULL;
   }

   // Spawn ferns on grass tiles first
// Spawn ferns and trees on grass tiles
for (int y = 0; y < GRID_SIZE; y++) {
    for (int x = 0; x < GRID_SIZE; x++) {
        if (isPositionInLoadedChunk(x, y)) {
            if (grid[y][x].terrainType == (uint8_t)TERRAIN_GRASS) {
                float random = (float)rand() / RAND_MAX;
                if (random < 0.1f) {  // 10% chance for fern
                    grid[y][x].structureType = STRUCTURE_PLANT;
                    grid[y][x].materialType = MATERIAL_FERN;
                    GRIDCELL_SET_WALKABLE(grid[y][x], false);

                }
                else if (random < 0.15f) {  // Additional 5% chance for tree
                    grid[y][x].structureType = STRUCTURE_PLANT;
                    grid[y][x].materialType = MATERIAL_TREE;
                    GRIDCELL_SET_WALKABLE(grid[y][x], false);
                }
            }
        }
    }
}

   printf("\n=== Starting Entity Initialization ===\n");
   printf("MAX_ENEMIES: %d\n", MAX_ENEMIES);
   printf("MAX_ENTITIES: %d\n", MAX_ENTITIES);

   printf("Player entity pointer stored at allEntities[0]: %p\n", (void*)allEntities[0]);

   // Then initialize enemies
   for (int i = 0; i < MAX_ENEMIES; i++) {
       int enemyGridX, enemyGridY;
       int attempts = 0;
       const int MAX_ATTEMPTS = 1000;
       bool validPosition = false;
       
       do {
           enemyGridX = rand() % GRID_SIZE;
           enemyGridY = rand() % GRID_SIZE;
           attempts++;
           
           if (isPositionInLoadedChunk(enemyGridX, enemyGridY) &&
               grid[enemyGridY][enemyGridX].structureType != STRUCTURE_WALL &&
               grid[enemyGridY][enemyGridX].structureType != STRUCTURE_PLANT &&
               GRIDCELL_IS_WALKABLE(grid[enemyGridY][enemyGridX])) {
               validPosition = true;
           }
           
           if (attempts >= MAX_ATTEMPTS) {
               fprintf(stderr, "Warning: Could not find valid spawn location for enemy %d after %d attempts\n", 
                       i, MAX_ATTEMPTS);
               enemyGridX = player.entity.gridX + (rand() % 3) - 1;
               enemyGridY = player.entity.gridY + (rand() % 3) - 1;
               break;
           }
       } while (!validPosition);

       InitEnemy(&enemies[i], enemyGridX, enemyGridY, MOVE_SPEED);
       allEntities[i + 1] = &enemies[i].entity;
   }

   ChunkCoord playerChunk = getChunkFromWorldPos(player.entity.gridX, player.entity.gridY);
   int radius = globalChunkManager->loadRadius;
   
   printf("\n=== Final Chunk Check ===\n");
   printf("Player chunk: (%d, %d)\n", playerChunk.x, playerChunk.y);
   printf("Load radius: %d\n", radius);
   
   // Final culling of chunks outside radius
   for (int cy = 0; cy < NUM_CHUNKS; cy++) {
       for (int cx = 0; cx < NUM_CHUNKS; cx++) {
           int dx = abs(cx - playerChunk.x);
           int dy = abs(cy - playerChunk.y);
           
           if (dx > radius || dy > radius) {
               int startX = cx * CHUNK_SIZE;
               int startY = cy * CHUNK_SIZE;
               
               for (int y = 0; y < CHUNK_SIZE; y++) {
                   for (int x = 0; x < CHUNK_SIZE; x++) {
                       int gridX = startX + x;
                       int gridY = startY + y;
                       if (gridX >= 0 && gridX < GRID_SIZE && 
                           gridY >= 0 && gridY < GRID_SIZE) {
                           grid[gridY][gridX].terrainType = (uint8_t)TERRAIN_UNLOADED;
                           GRIDCELL_SET_WALKABLE(grid[gridY][gridX], false);
                       }
                   }
               }
           }
       }
   }
   
   printf("Initial chunk culling complete.\n");
   printf("Game state initialization complete.\n");
}

void InitializeEngine(void) {
    printf("Initializing engine systems...\n");
    
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

    // Create shader programs
    shaderProgram = createShaderProgram();
    outlineShaderProgram = createOutlineShaderProgram();
    initUIResources();  // Initialize UI resources
        initializeCrateUIRenderer();  // Add this line
    printf("Crate UI renderer initialized.\n");
    itemShaderProgram = createItemShaderProgram();
    
    if (!shaderProgram || !outlineShaderProgram || !getUIShaderProgram() || !itemShaderProgram) {
        fprintf(stderr, "Failed to create shader programs\n");
        SDL_GL_DeleteContext(mainContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(1);
    }
    printf("Shader programs created.\n");

    // Load texture atlas first
    textureAtlas = loadBMP("sprite-sheet-32.bmp");
    initializeDefaultTextures();  // Add this line
    printf("Texture system initialized.\n");
    if (!textureAtlas) {
        fprintf(stderr, "Failed to load texture atlas\n");
        glDeleteProgram(shaderProgram);
        glDeleteProgram(outlineShaderProgram);
        glDeleteProgram(getUIShaderProgram());
        glDeleteProgram(itemShaderProgram);
        SDL_GL_DeleteContext(mainContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(1);
    }
    printf("Texture atlas loaded with ID: %u\n", textureAtlas);

    // Set up uniform locations
    colorUniform = glGetUniformLocation(shaderProgram, "color");
    textureUniform = glGetUniformLocation(shaderProgram, "textureAtlas");

    // Create grid VAO
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

    // Create Square VAO & VBO
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

    // Initialize specialized VAOs
    initializeEnemyBatchVAO();
    printf("Enemy batch VAO initialized.\n");

    initializeOutlineVAO();
    printf("Outline VAO initialized.\n");

    initializeTilesBatchVAO();
    printf("Tiles batch VAO initialized.\n");

    initializeGPUPathfinding();
    printf("GPU pathfinding initialized.\n");

    // Initialize UI system with shader and texture atlas
    InitializeUI(getUIShaderProgram());
    glUseProgram(getUIShaderProgram());
    GLint uiTextureLoc = glGetUniformLocation(getUIShaderProgram(), "textureAtlas");
    glUniform1i(uiTextureLoc, 0);  // Use texture unit 0
    printf("UI system initialized with texture atlas.\n");

    initializeViewports();
    printf("Viewports initialized.\n");

    // Check for any GL errors after initialization
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        printf("GL error after initialization: 0x%x\n", err);
    }
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

bool LoadGame(const char* filename) {
    printf("START LoadGame sequence\n");
    InitializeEngine();
    printf("After InitializeEngine\n");
    InitializeGameState(false);  // false = loading save
    printf("After InitializeGameState\n");
    bool result = loadGameState(filename);
    printf("After loadGameState\n");
    return result;
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
    printf("Starting cleanup sequence...\n");
    
    // 1. Cleanup game systems first
    CleanupEntities();
    CleanupUI();
    cleanupEnclosureManager(&globalEnclosureManager);
    cleanupStorageManager(&globalStorageManager);  // Add this
    
    printf("Game systems cleaned up.\n");

    // 2. Cleanup rendering resources
    cleanupUIResources();
    cleanupCrateUIRenderer();
    // 3. Delete VAOs
    if (gridVAO) {
        glDeleteVertexArrays(1, &gridVAO);
        gridVAO = 0;
    }
    if (squareVAO) {
        glDeleteVertexArrays(1, &squareVAO);
        squareVAO = 0;
    }
    if (outlineVAO) {
        glDeleteVertexArrays(1, &outlineVAO);
        outlineVAO = 0;
    }
    if (enemyBatchVAO) {
        glDeleteVertexArrays(1, &enemyBatchVAO);
        enemyBatchVAO = 0;
    }
    if (tilesBatchVAO) {
        glDeleteVertexArrays(1, &tilesBatchVAO);
        tilesBatchVAO = 0;
    }
    
    printf("VAOs cleaned up.\n");

    // 4. Delete VBOs
    if (squareVBO) {
        glDeleteBuffers(1, &squareVBO);
        squareVBO = 0;
    }
    if (outlineVBO) {
        glDeleteBuffers(1, &outlineVBO);
        outlineVBO = 0;
    }
    if (enemyBatchVBO) {
        glDeleteBuffers(1, &enemyBatchVBO);
        enemyBatchVBO = 0;
    }
    if (tilesBatchVBO) {
        glDeleteBuffers(1, &tilesBatchVBO);
        tilesBatchVBO = 0;
    }
    
    printf("VBOs cleaned up.\n");

    // 5. Delete shader programs
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
    if (outlineShaderProgram) {
        glDeleteProgram(outlineShaderProgram);
        outlineShaderProgram = 0;
    }
    if (itemShaderProgram) {
        glDeleteProgram(itemShaderProgram);
        itemShaderProgram = 0;
    }
    
    printf("Shader programs cleaned up.\n");

    // 6. Clean up batch data
    cleanupEntityBatchData();
    cleanupTileBatchData();
    
    printf("Batch data cleaned up.\n");

    // 7. Clean up texture resources
    if (textureAtlas) {
        glDeleteTextures(1, &textureAtlas);
        textureAtlas = 0;
    }
    cleanupTextureManager();
    
    printf("Texture resources cleaned up.\n");

    // 8. Clean up chunk management
    if (globalChunkManager) {
        cleanupChunkManager(globalChunkManager);
        free(globalChunkManager);
        globalChunkManager = NULL;
    }
    
    printf("Chunk manager cleaned up.\n");

    // 9. Clean up grid resources
    cleanupGrid();
    
    printf("Grid cleaned up.\n");

    // 10. Clean up GPU resources
    cleanupGPUPathfinding();
    
    printf("GPU resources cleaned up.\n");

    // 11. Finally, cleanup SDL resources
    if (mainContext) {
        SDL_GL_DeleteContext(mainContext);
        mainContext = NULL;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    SDL_Quit();
    
    printf("SDL resources cleaned up.\n");
    printf("Cleanup sequence complete.\n");
}
bool westIsCorner(int x, int y) {
    if (x <= 0) return false;
    if (grid[y][x-1].structureType != STRUCTURE_WALL) return false;

    
    float texY = grid[y][x-1].wallTexY;
    return (texY == 0.0f/4.0f) ||    // Top corners row
           (texY == 1.0f/4.0f);      // Bottom corners row
}

bool eastIsCorner(int x, int y) {
    if (x >= GRID_SIZE-1) return false; 
    if (grid[y][x+1].structureType != STRUCTURE_WALL) return false;

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
    // Increment game tick
    atomic_fetch_add(&game_ticks, 1);
    
    // Update entities as before
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (isPositionInLoadedChunk(enemies[i].entity.posX, enemies[i].entity.posY)) {
            UpdateEnemy(&enemies[i], allEntities, MAX_ENTITIES, SDL_GetTicks());
        }
    }
}

/*
 * Render
 *
 * Renders the game world including tiles and entities.
 */
void Render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1. Render game view
    applyViewport(&gameViewport);
    glUseProgram(shaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureAtlas);
    glUniform1i(textureUniform, 0);

    float cameraOffsetX = player.cameraCurrentX;
    float cameraOffsetY = player.cameraCurrentY;
    float zoomFactor = player.zoomFactor;

    RenderTiles(cameraOffsetX, cameraOffsetY, zoomFactor);  // Terrain and structures only
    RenderEntities(cameraOffsetX, cameraOffsetY, zoomFactor);
    RenderTreeCanopies(cameraOffsetX, cameraOffsetY, zoomFactor);  // Now canopies render on top
    renderStructurePreview(&placementMode, cameraOffsetX, cameraOffsetY, zoomFactor);
    // Check all crates in the storage manager
    RenderCrateUIs(cameraOffsetX, cameraOffsetY, zoomFactor);

    // 2. Render UI elements in sidebar
    applyViewport(&sidebarViewport);
    RenderUI(&player);

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


void RenderTreeCanopies(float cameraOffsetX, float cameraOffsetY, float zoomFactor) {
    TextureCoords* canopyTex = getTextureCoords("tree_canopy");
    if (!canopyTex || !tileBatchData.persistentBuffer) return;
    glUseProgram(shaderProgram);
    glBindVertexArray(tilesBatchVAO);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    float* batchData = tileBatchData.persistentBuffer;
    int dataIndex = 0;
    int renderedTiles = 0;
    const float texMargin = 0.0000001f;
    float playerWorldX = player.entity.posX;
    float playerWorldY = player.entity.posY;

    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
        if (grid[y][x].terrainType == TERRAIN_UNLOADED || 
            y > 0 && grid[y-1][x].terrainType == TERRAIN_UNLOADED) {
            continue;
        }

            if (grid[y][x].structureType == STRUCTURE_PLANT && 
                grid[y][x].materialType == MATERIAL_TREE) {
                
                float worldX, worldY;
                WorldToScreenCoords(x, y - 1, 0, 0, 1, &worldX, &worldY);

                if (!isPointVisible(worldX, worldY, playerWorldX, playerWorldY, zoomFactor)) {
                    continue;
                }
                
                float screenX, screenY;
                WorldToScreenCoords(x, y - 1, cameraOffsetX, cameraOffsetY, zoomFactor, &screenX, &screenY);
                
                float halfSize = TILE_SIZE * zoomFactor;

                // First triangle
                batchData[dataIndex++] = screenX - halfSize;
                batchData[dataIndex++] = screenY - halfSize;
                batchData[dataIndex++] = canopyTex->u1 + texMargin;
                batchData[dataIndex++] = canopyTex->v1 + texMargin;

                batchData[dataIndex++] = screenX + halfSize;
                batchData[dataIndex++] = screenY - halfSize;
                batchData[dataIndex++] = canopyTex->u2 - texMargin;
                batchData[dataIndex++] = canopyTex->v1 + texMargin;

                batchData[dataIndex++] = screenX + halfSize;
                batchData[dataIndex++] = screenY + halfSize;
                batchData[dataIndex++] = canopyTex->u2 - texMargin;
                batchData[dataIndex++] = canopyTex->v2 - texMargin;

                // Second triangle
                batchData[dataIndex++] = screenX - halfSize;
                batchData[dataIndex++] = screenY - halfSize;
                batchData[dataIndex++] = canopyTex->u1 + texMargin;
                batchData[dataIndex++] = canopyTex->v1 + texMargin;

                batchData[dataIndex++] = screenX + halfSize;
                batchData[dataIndex++] = screenY + halfSize;
                batchData[dataIndex++] = canopyTex->u2 - texMargin;
                batchData[dataIndex++] = canopyTex->v2 - texMargin;

                batchData[dataIndex++] = screenX - halfSize;
                batchData[dataIndex++] = screenY + halfSize;
                batchData[dataIndex++] = canopyTex->u1 + texMargin;
                batchData[dataIndex++] = canopyTex->v2 - texMargin;

                renderedTiles++;
            }
        }
    }

    if (dataIndex > 0) {
        glBindBuffer(GL_ARRAY_BUFFER, tilesBatchVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, dataIndex * sizeof(float), batchData);
        glDrawArrays(GL_TRIANGLES, 0, renderedTiles * 6);
    }
    
    glDisable(GL_BLEND);
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
    const float texMargin = 0.0000001f;

    // Main tile and structure rendering pass
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

            float posX, posY;
            WorldToScreenCoords(x, y, cameraOffsetX, cameraOffsetY, zoomFactor, &posX, &posY);
            float halfSize = TILE_SIZE * zoomFactor;

            // Get terrain texture coordinates with variation handling
            const char* terrainId = NULL;
            
            // Handle terrain variations
            if (grid[y][x].terrainType == TERRAIN_GRASS) {
                uint16_t flags = grid[y][x].flags;
                uint16_t variation = (flags & TERRAIN_VARIATION_MASK) >> 8;
                switch(variation) {
                    case 0: terrainId = "terrain_grass"; break;
                    case 1: terrainId = "terrain_grass_2"; break;
                    case 2: terrainId = "terrain_grass_3"; break;
                    case 3: terrainId = "terrain_grass_4"; break;
                    case 4: terrainId = "terrain_grass_4"; break;
                    default: terrainId = "terrain_grass"; break;
                }
            } 
            else if (grid[y][x].terrainType == TERRAIN_STONE) {
                uint16_t flags = grid[y][x].flags;
                uint16_t variation = (flags & TERRAIN_VARIATION_MASK) >> 8;
                switch(variation) {
                    case 0: terrainId = "terrain_stone_2"; break;
                    case 1: terrainId = "terrain_stone_3"; break;
                    case 2: terrainId = "terrain_stone_4"; break;
                    case 3: terrainId = "terrain_stone_2"; break;
                    default: terrainId = "terrain_stone"; break;
                }
            }
            else {
                switch (grid[y][x].terrainType) {
                    case TERRAIN_SAND:    terrainId = "terrain_sand"; break;
                    case TERRAIN_WATER:   terrainId = "terrain_water"; break;
                    default:              terrainId = "terrain_grass"; break;
                }
            }

            TextureCoords* baseCoords = getTextureCoords(terrainId);
            if (!baseCoords) continue;

            // Vertex positions for each corner (TL, TR, BR, BL)
            float vertices[4][2] = {
                {posX - halfSize, posY - halfSize},  // Top-Left
                {posX + halfSize, posY - halfSize},  // Top-Right
                {posX + halfSize, posY + halfSize},  // Bottom-Right
                {posX - halfSize, posY + halfSize}   // Bottom-Left
            };

            // UV coordinates for each corner
            float uvs[4][2] = {
                {baseCoords->u1 + texMargin, baseCoords->v1 + texMargin},     // Top-Left
                {baseCoords->u2 - texMargin, baseCoords->v1 + texMargin},     // Top-Right
                {baseCoords->u2 - texMargin, baseCoords->v2 - texMargin},     // Bottom-Right
                {baseCoords->u1 + texMargin, baseCoords->v2 - texMargin}      // Bottom-Left
            };

            // Apply rotation - rotate vertex indices
            int rotatedIndices[4];
            uint8_t terrainRotation = GRIDCELL_GET_TERRAIN_ROTATION(grid[y][x]);
            switch(terrainRotation & 3) {
                case 0:  // No rotation
                    rotatedIndices[0] = 0; rotatedIndices[1] = 1;
                    rotatedIndices[2] = 2; rotatedIndices[3] = 3;
                    break;
                case 1:  // 90 degrees clockwise
                    rotatedIndices[0] = 3; rotatedIndices[1] = 0;
                    rotatedIndices[2] = 1; rotatedIndices[3] = 2;
                    break;
                case 2:  // 180 degrees
                    rotatedIndices[0] = 2; rotatedIndices[1] = 3;
                    rotatedIndices[2] = 0; rotatedIndices[3] = 1;
                    break;
                case 3:  // 270 degrees clockwise
                    rotatedIndices[0] = 1; rotatedIndices[1] = 2;
                    rotatedIndices[2] = 3; rotatedIndices[3] = 0;
                    break;
            }

            // First triangle with rotation
            batchData[dataIndex++] = vertices[rotatedIndices[0]][0];
            batchData[dataIndex++] = vertices[rotatedIndices[0]][1];
            batchData[dataIndex++] = uvs[0][0];
            batchData[dataIndex++] = uvs[0][1];

            batchData[dataIndex++] = vertices[rotatedIndices[1]][0];
            batchData[dataIndex++] = vertices[rotatedIndices[1]][1];
            batchData[dataIndex++] = uvs[1][0];
            batchData[dataIndex++] = uvs[1][1];

            batchData[dataIndex++] = vertices[rotatedIndices[2]][0];
            batchData[dataIndex++] = vertices[rotatedIndices[2]][1];
            batchData[dataIndex++] = uvs[2][0];
            batchData[dataIndex++] = uvs[2][1];

            // Second triangle with rotation
            batchData[dataIndex++] = vertices[rotatedIndices[0]][0];
            batchData[dataIndex++] = vertices[rotatedIndices[0]][1];
            batchData[dataIndex++] = uvs[0][0];
            batchData[dataIndex++] = uvs[0][1];

            batchData[dataIndex++] = vertices[rotatedIndices[2]][0];
            batchData[dataIndex++] = vertices[rotatedIndices[2]][1];
            batchData[dataIndex++] = uvs[2][0];
            batchData[dataIndex++] = uvs[2][1];

            batchData[dataIndex++] = vertices[rotatedIndices[3]][0];
            batchData[dataIndex++] = vertices[rotatedIndices[3]][1];
            batchData[dataIndex++] = uvs[3][0];
            batchData[dataIndex++] = uvs[3][1];

            renderedTiles++;

            // Render structures
            if (grid[y][x].structureType != 0) {
                TextureCoords* structureTex = NULL;
                
if (grid[y][x].structureType == STRUCTURE_WALL) {
    // Get the texture coordinates for this wall from stored values
    float u1 = grid[y][x].wallTexX;
    float v1 = grid[y][x].wallTexY;
    
    // Get the matching texture u2/v2
    TextureCoords* structureTex = getTextureCoords("wall_vertical"); // Use any wall texture to get correct width/height
    float u2 = u1 + (structureTex->u2 - structureTex->u1);
    float v2 = v1 + (structureTex->v2 - structureTex->v1);

    // First triangle
    batchData[dataIndex++] = posX - halfSize;
    batchData[dataIndex++] = posY - halfSize;
    batchData[dataIndex++] = u1 + texMargin;
    batchData[dataIndex++] = v1 + texMargin;

    batchData[dataIndex++] = posX + halfSize;
    batchData[dataIndex++] = posY - halfSize;
    batchData[dataIndex++] = u2 - texMargin;
    batchData[dataIndex++] = v1 + texMargin;

    batchData[dataIndex++] = posX + halfSize;
    batchData[dataIndex++] = posY + halfSize;
    batchData[dataIndex++] = u2 - texMargin;
    batchData[dataIndex++] = v2 - texMargin;

    // Second triangle
    batchData[dataIndex++] = posX - halfSize;
    batchData[dataIndex++] = posY - halfSize;
    batchData[dataIndex++] = u1 + texMargin;
    batchData[dataIndex++] = v1 + texMargin;

    batchData[dataIndex++] = posX + halfSize;
    batchData[dataIndex++] = posY + halfSize;
    batchData[dataIndex++] = u2 - texMargin;
    batchData[dataIndex++] = v2 - texMargin;

    batchData[dataIndex++] = posX - halfSize;
    batchData[dataIndex++] = posY + halfSize;
    batchData[dataIndex++] = u1 + texMargin;
    batchData[dataIndex++] = v2 - texMargin;

    renderedTiles++;
}
                else if (grid[y][x].structureType == STRUCTURE_DOOR) {
                    bool isOpen = GRIDCELL_IS_WALKABLE(grid[y][x]);
                    bool isVertical = (GRIDCELL_GET_ORIENTATION(grid[y][x]) == 0);
                    
                    if (isVertical) {
                        structureTex = getTextureCoords(isOpen ? "door_vertical_open" : "door_vertical");
                    } else {
                        structureTex = getTextureCoords(isOpen ? "door_horizontal_open" : "door_horizontal");
                    }
                }
                else if (grid[y][x].structureType == STRUCTURE_PLANT) {
                    if (grid[y][x].materialType == MATERIAL_FERN) {
                        structureTex = getTextureCoords("item_fern");
                    }
                    else if (grid[y][x].materialType == MATERIAL_TREE) {
                        structureTex = getTextureCoords("tree_trunk");
                    }
                }
                    else if (grid[y][x].structureType == STRUCTURE_CRATE) {
        structureTex = getTextureCoords("item_plant_crate");
    }

                if (structureTex) {
                    // First triangle for structure
                    batchData[dataIndex++] = posX - halfSize;
                    batchData[dataIndex++] = posY - halfSize;
                    batchData[dataIndex++] = structureTex->u1 + texMargin;
                    batchData[dataIndex++] = structureTex->v1 + texMargin;

                    batchData[dataIndex++] = posX + halfSize;
                    batchData[dataIndex++] = posY - halfSize;
                    batchData[dataIndex++] = structureTex->u2 - texMargin;
                    batchData[dataIndex++] = structureTex->v1 + texMargin;

                    batchData[dataIndex++] = posX + halfSize;
                    batchData[dataIndex++] = posY + halfSize;
                    batchData[dataIndex++] = structureTex->u2 - texMargin;
                    batchData[dataIndex++] = structureTex->v2 - texMargin;

                    // Second triangle for structure
                    batchData[dataIndex++] = posX - halfSize;
                    batchData[dataIndex++] = posY - halfSize;
                    batchData[dataIndex++] = structureTex->u1 + texMargin;
                    batchData[dataIndex++] = structureTex->v1 + texMargin;

                    batchData[dataIndex++] = posX + halfSize;
                    batchData[dataIndex++] = posY + halfSize;
                    batchData[dataIndex++] = structureTex->u2 - texMargin;
                    batchData[dataIndex++] = structureTex->v2 - texMargin;

                    batchData[dataIndex++] = posX - halfSize;
                    batchData[dataIndex++] = posY + halfSize;
                    batchData[dataIndex++] = structureTex->u1 + texMargin;
                    batchData[dataIndex++] = structureTex->v2 - texMargin;

                    renderedTiles++;
                }
            }
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, tilesBatchVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, dataIndex * sizeof(float), batchData);
    glDrawArrays(GL_TRIANGLES, 0, renderedTiles * 6);

    if (isPointVisible(player.entity.finalGoalX, player.entity.finalGoalY, playerWorldX, playerWorldY, zoomFactor)) {
        drawTargetTileOutline(player.entity.finalGoalX, player.entity.finalGoalY, cameraOffsetX, cameraOffsetY, zoomFactor);
    }
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

    // Process enemies in groups of 4 for SIMD
    int i;
    for (i = 0; i < MAX_ENEMIES - 3; i += 4) {  // Process full groups of 4
        bool enemyValid[4] = {false, false, false, false};

        // Check chunk loading for all 4
        for (int j = 0; j < 4; j++) {
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

        for (int j = 0; j < 4; j++) {
            if (enemyValid[j] && (visibilityMask & (1 << j))) {
                visibleEnemies[visibleEnemyCount++] = enemies[i + j];
            } else if (enemyValid[j]) {
                culledEnemyCount++;
            }
        }
    }

    // Handle remaining enemies individually
    for (; i < MAX_ENEMIES; i++) {
        if (isPositionInLoadedChunk(enemies[i].entity.posX, enemies[i].entity.posY)) {
            float screenX = (enemies[i].entity.posX - playerWorldX) * zoomFactor;
            float screenY = (enemies[i].entity.posY - playerWorldY) * zoomFactor;
            
            if (screenX >= -1.0f - TILE_SIZE && screenX <= 1.0f + TILE_SIZE &&
                screenY >= -1.0f - TILE_SIZE && screenY <= 1.0f + TILE_SIZE) {
                visibleEnemies[visibleEnemyCount++] = enemies[i];
            } else {
                culledEnemyCount++;
            }
        } else {
            chunkCulledCount++;
        }
    }

    float smoothCameraOffsetX = player.cameraCurrentX;
    float smoothCameraOffsetY = player.cameraCurrentY;

    // Update enemy batch VBO and render
    updateEnemyBatchVBO(visibleEnemies, visibleEnemyCount, smoothCameraOffsetX, smoothCameraOffsetY, zoomFactor);
    glBindVertexArray(enemyBatchVAO);
    glDrawArrays(GL_TRIANGLES, 0, visibleEnemyCount * 6);

    // Render player
    glBindVertexArray(squareVAO);
    glBindBuffer(GL_ARRAY_BUFFER, squareVBO);

    float playerScreenX = (playerWorldX - smoothCameraOffsetX) * zoomFactor;
    float playerScreenY = (playerWorldY - smoothCameraOffsetY) * zoomFactor;

    // Get player texture based on animation state and direction
    TextureCoords* playerTex;
    char textureName[32];

    if (!player.animation->isMoving) {
        // Use standing frame based on direction
        switch(player.animation->facing) {
        case DIRECTION_UP:
            playerTex = getTextureCoords("player_run_up_0");
            break;
        case DIRECTION_DOWN:
            playerTex = getTextureCoords("player");  // Original standing frame for down
            break;
        case DIRECTION_LEFT:
            playerTex = getTextureCoords("player_run_left_0");
            break;
        case DIRECTION_RIGHT:
            playerTex = getTextureCoords("player_run_right_0");
            break;
        default:
            playerTex = getTextureCoords("player");  // Default to original standing frame
        }
    } else {
        // Get running animation frame based on direction
        const char* dirStr;
        switch(player.animation->facing) {
            case DIRECTION_UP:
                dirStr = "up";
                break;
            case DIRECTION_DOWN:
                dirStr = "down";
                break;
            case DIRECTION_LEFT:
                dirStr = "left";
                break;
            case DIRECTION_RIGHT:
                dirStr = "right";
                break;
            default:
                dirStr = "down";
        }
        snprintf(textureName, sizeof(textureName), "player_run_%s_%d", 
                dirStr, player.animation->currentFrame);
        playerTex = getTextureCoords(textureName);
    }

    if (!playerTex) {
        fprintf(stderr, "Failed to get player texture coordinates\n");
        return;
    }

    float playerVertices[] = {
        playerScreenX - TILE_SIZE * zoomFactor, playerScreenY - TILE_SIZE * zoomFactor, 
        playerTex->u1, playerTex->v1,
        playerScreenX + TILE_SIZE * zoomFactor, playerScreenY - TILE_SIZE * zoomFactor, 
        playerTex->u2, playerTex->v1,
        playerScreenX + TILE_SIZE * zoomFactor, playerScreenY + TILE_SIZE * zoomFactor, 
        playerTex->u2, playerTex->v2,
        playerScreenX - TILE_SIZE * zoomFactor, playerScreenY + TILE_SIZE * zoomFactor, 
        playerTex->u1, playerTex->v2
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

        // Get the current time once for all enemies
        Uint32 currentTime = SDL_GetTicks();

        // Update other entities - but only if they're in loaded chunks
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (isPositionInLoadedChunk(enemies[i].entity.posX, enemies[i].entity.posY)) {
                UpdateEntity(&enemies[i].entity, allEntities, MAX_ENTITIES);
                UpdateEnemy(&enemies[i], allEntities, MAX_ENTITIES, currentTime);
            }
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
