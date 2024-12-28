// grid.c

#include "grid.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "asciiMap.h" 
GridCell grid[GRID_SIZE][GRID_SIZE];

BiomeData biomeData[BIOME_COUNT] = {
    {{TERRAIN_WATER, TERRAIN_SAND, TERRAIN_STONE}, {0.3f, 0.1f}},    // OCEAN
    {{TERRAIN_SAND, TERRAIN_SAND, TERRAIN_STONE}, {0.6f, 0.3f}},     // BEACH
    {{TERRAIN_GRASS, TERRAIN_DIRT, TERRAIN_STONE}, {0.7f, 0.4f}},    // PLAINS
    {{TERRAIN_GRASS, TERRAIN_DIRT, TERRAIN_STONE}, {0.8f, 0.5f}},    // FOREST
    {{TERRAIN_SAND, TERRAIN_SAND, TERRAIN_STONE}, {0.6f, 0.3f}},     // DESERT
    {{TERRAIN_GRASS, TERRAIN_STONE, TERRAIN_STONE}, {0.9f, 0.7f}}    // MOUNTAINS
};

#define UNWALKABLE_PROBABILITY 0.04f  // Define the unwalkable probability

// Perlin noise function declarations
float noise(int x, int y);
float smoothNoise(int x, int y);
float interpolatedNoise(float x, float y);
float perlinNoise(float x, float y, float persistence, int octaves);

/*
 * initializeGrid
 *
 * Initializes the grid with default terrain and biome types.
 *
 * @param[in] size The size of the grid to initialize
 */
void initializeGrid(int size) {
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            grid[y][x].terrainType = TERRAIN_GRASS;
            grid[y][x].biomeType = BIOME_PLAINS;
            grid[y][x].isWalkable = true;
        }
    }
}

/*
 * cleanupGrid
 *
 * Cleans up resources allocated for the grid. (Currently does nothing)
 */
void cleanupGrid() {
    if (loadedMapData) {
        free(loadedMapData);
        loadedMapData = NULL;
    }
}

/*
 * isWalkable
 *
 * Checks if a given grid cell is walkable.
 *
 * @param[in] x The x-coordinate of the grid cell
 * @param[in] y The y-coordinate of the grid cell
 * @return bool True if the cell is walkable, false otherwise
 */
bool isWalkable(int x, int y) {
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) {
        return false;
    }
    return grid[y][x].isWalkable;
}

/*
 * isValid
 *
 * Checks if a given grid cell is within valid grid bounds.
 *
 * @param[in] x The x-coordinate of the grid cell
 * @param[in] y The y-coordinate of the grid cell
 * @return bool True if the cell is within bounds, false otherwise
 */
bool isValid(int x, int y) {
    return x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE;
}

void processChunk(Chunk* chunk) {
    if (chunk->chunkX < 0 || chunk->chunkX >= NUM_CHUNKS || 
        chunk->chunkY < 0 || chunk->chunkY >= NUM_CHUNKS) {
        printf("Invalid chunk coordinates: %d, %d\n", chunk->chunkX, chunk->chunkY);
        return;
    }
    
    // Process chunk data - for now just validates coordinates
    // Future: Add chunk-specific processing here
}

/*
 * setGridSize
 *
 * Sets the grid size. (Currently does nothing as the grid size is fixed)
 *
 * @param[in] size The desired grid size
 */
void setGridSize(int size) {
    // We're using a fixed size grid for now, so this function doesn't do much
    if (size != GRID_SIZE) {
        printf("Warning: Attempted to set grid size to %d, but GRID_SIZE is fixed at %d\n", size, GRID_SIZE);
    }
}

void debugPrintGridSection(int startX, int startY, int width, int height) {
    printf("\nGrid Section from (%d,%d):\n", startX, startY);
    for(int y = startY; y < startY + height && y < GRID_SIZE; y++) {
        for(int x = startX; x < startX + width && x < GRID_SIZE; x++) {
            printf("%c ", terrainToChar(grid[y][x].terrainType));
        }
        printf("\n");
    }
    printf("\n");
}

// Modify writeChunkToGrid in grid.c:
void writeChunkToGrid(const Chunk* chunk) {
    int startX = chunk->chunkX * CHUNK_SIZE;
    int startY = chunk->chunkY * CHUNK_SIZE;
    
    printf("\nWriting chunk (%d,%d) to grid starting at position (%d,%d)\n", 
           chunk->chunkX, chunk->chunkY, startX, startY);
    
    for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            int gridX = startX + x;
            int gridY = startY + y;
            
            if (gridX < GRID_SIZE && gridY < GRID_SIZE) {
                grid[gridY][gridX] = chunk->cells[y][x];
            }
        }
    }
}


void generateTerrain() {
    printf("Generating initial terrain...\n");
    
    // Initialize each chunk in the grid
    for (int chunkY = 0; chunkY < NUM_CHUNKS; chunkY++) {
        for (int chunkX = 0; chunkX < NUM_CHUNKS; chunkX++) {
            Chunk tempChunk;
            initializeChunk(&tempChunk, chunkX, chunkY);
            writeChunkToGrid(&tempChunk);
        }
    }

    printf("Initial terrain generation complete\n");

    // Ensure the center of the map is walkable for player spawn
    int centerX = GRID_SIZE / 2;
    int centerY = GRID_SIZE / 2;
    for (int y = centerY - 1; y <= centerY + 1; y++) {
        for (int x = centerX - 1; x <= centerX + 1; x++) {
            if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE) {
                grid[y][x].isWalkable = true;
                if (grid[y][x].terrainType == TERRAIN_WATER || 
                    grid[y][x].terrainType == TERRAIN_UNWALKABLE) {
                    grid[y][x].terrainType = TERRAIN_GRASS;
                }
            }
        }
    }
}
// Perlin noise functions

/*
 * noise
 *
 * Generates a noise value for a given coordinate.
 *
 * @param[in] x The x-coordinate
 * @param[in] y The y-coordinate
 * @return float The noise value
 */
float noise(int x, int y) {
    int n = x + y * 57;
    n = (n << 13) ^ n;
    return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589 + rand()) & 0x7fffffff) / 1073741824.0f);
}

/*
 * smoothNoise
 *
 * Generates a smoothed noise value for a given coordinate.
 *
 * @param[in] x The x-coordinate
 * @param[in] y The y-coordinate
 * @return float The smoothed noise value
 */
float smoothNoise(int x, int y) {
    float corners = (noise(x-1, y-1) + noise(x+1, y-1) + noise(x-1, y+1) + noise(x+1, y+1)) / 16.0f;
    float sides = (noise(x-1, y) + noise(x+1, y) + noise(x, y-1) + noise(x, y+1)) / 8.0f;
    float center = noise(x, y) / 4.0f;
    return corners + sides + center;
}

/*
 * interpolatedNoise
 *
 * Generates an interpolated noise value for a given coordinate.
 *
 * @param[in] x The x-coordinate
 * @param[in] y The y-coordinate
 * @return float The interpolated noise value
 */
float interpolatedNoise(float x, float y) {
    int intX = (int)x;
    float fracX = x - intX;
    int intY = (int)y;
    float fracY = y - intY;

    float v1 = smoothNoise(intX, intY);
    float v2 = smoothNoise(intX + 1, intY);
    float v3 = smoothNoise(intX, intY + 1);
    float v4 = smoothNoise(intX + 1, intY + 1);

    float i1 = v1 * (1 - fracX) + v2 * fracX;
    float i2 = v3 * (1 - fracX) + v4 * fracX;

    return i1 * (1 - fracY) + i2 * fracY;
}

/*
 * perlinNoise
 *
 * Generates Perlin noise for a given coordinate using multiple octaves.
 *
 * @param[in] x The x-coordinate
 * @param[in] y The y-coordinate
 * @param[in] persistence The persistence value
 * @param[in] octaves The number of octaves
 * @return float The Perlin noise value
 */
float perlinNoise(float x, float y, float persistence, int octaves) {
    float total = 0;
    float frequency = 1;
    float amplitude = 1;
    float maxValue = 0;

    for(int i = 0; i < octaves; i++) {
        total += interpolatedNoise(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2;
    }

    return total / maxValue;
}

// Global chunk manager
ChunkManager* globalChunkManager = NULL;

void initChunkManager(ChunkManager* manager, int loadRadius) {
    manager->loadRadius = loadRadius;
    manager->numLoadedChunks = 0;
    manager->playerChunk.x = -9999;  // Invalid initial position
    manager->playerChunk.y = -9999;

    // Initialize all chunk pointers to NULL
    for (int i = 0; i < MAX_LOADED_CHUNKS; i++) {
        manager->chunks[i] = NULL;
        manager->chunkCoords[i].x = 0;
        manager->chunkCoords[i].y = 0;
    }

    printf("Chunk manager initialized with radius %d\n", loadRadius);
}

void initializeChunk(Chunk* chunk, int chunkX, int chunkY) {
    chunk->chunkX = chunkX;
    chunk->chunkY = chunkY;
    chunk->isLoaded = true;
    
    // Initialize with default terrain (will be overwritten by map data)
    for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            chunk->cells[y][x].terrainType = TERRAIN_GRASS;
            chunk->cells[y][x].biomeType = BIOME_PLAINS;
            chunk->cells[y][x].isWalkable = true;
        }
    }

    printf("Initialized chunk (%d,%d)\n", chunkX, chunkY);
}
void cleanupChunkManager(ChunkManager* manager) {
    if (!manager) return;

    for (int i = 0; i < MAX_LOADED_CHUNKS; i++) {
        if (manager->chunks[i]) {
            free(manager->chunks[i]);
            manager->chunks[i] = NULL;
        }
    }

    manager->numLoadedChunks = 0;
    printf("Chunk manager cleaned up\n");
}

ChunkCoord getChunkFromWorldPos(float worldX, float worldY) {
    ChunkCoord coord;
    
    // Convert from normalized (-1 to 1) to grid coordinates (0 to GRID_SIZE)
    float gridX = (worldX + 1.0f) * (GRID_SIZE / 2.0f);
    float gridY = (-worldY + 1.0f) * (GRID_SIZE / 2.0f);
    
    // Convert to chunk coordinates
    coord.x = (int)floorf(gridX / CHUNK_SIZE);
    coord.y = (int)floorf(gridY / CHUNK_SIZE);
    
    printf("World pos (%.2f, %.2f) -> Grid pos (%.2f, %.2f) -> Chunk (%d, %d)\n", 
           worldX, worldY, gridX, gridY, coord.x, coord.y);
           
    return coord;
}

bool isChunkLoaded(ChunkManager* manager, int chunkX, int chunkY) {
    for (int i = 0; i < manager->numLoadedChunks; i++) {
        if (manager->chunkCoords[i].x == chunkX && 
            manager->chunkCoords[i].y == chunkY) {
            return true;
        }
    }
    return false;
}

Chunk* getChunk(ChunkManager* manager, int chunkX, int chunkY) {
    for (int i = 0; i < manager->numLoadedChunks; i++) {
        if (manager->chunkCoords[i].x == chunkX && 
            manager->chunkCoords[i].y == chunkY) {
            return manager->chunks[i];
        }
    }
    return NULL;
}

void updatePlayerChunk(ChunkManager* manager, float playerX, float playerY) {
    ChunkCoord currentChunk = getChunkFromWorldPos(playerX, playerY);
    
    // Calculate position within current chunk (0.0 to 1.0)
    float chunkRelativeX = (playerX / CHUNK_SIZE) - currentChunk.x;
    float chunkRelativeY = (playerY / CHUNK_SIZE) - currentChunk.y;
    
    // Define edge threshold (e.g., when within 20% of chunk edge)
    float edgeThreshold = 0.2f;
    
    // Check if near any chunk edge
    bool nearEdge = (chunkRelativeX < edgeThreshold || 
                    chunkRelativeX > (1.0f - edgeThreshold) ||
                    chunkRelativeY < edgeThreshold || 
                    chunkRelativeY > (1.0f - edgeThreshold));

    // Trigger chunk loading if:
    // 1. Player moved to new chunk OR
    // 2. Player is near edge of current chunk
    if (currentChunk.x != manager->playerChunk.x || 
        currentChunk.y != manager->playerChunk.y ||
        nearEdge) {
        
        printf("Chunk update triggered - Position: (%.2f, %.2f), In chunk: (%d, %d), Near edge: %s\n",
               playerX, playerY, currentChunk.x, currentChunk.y, 
               nearEdge ? "true" : "false");
        
        manager->playerChunk = currentChunk;
        loadChunksAroundPlayer(manager);
    }
}
void loadChunksAroundPlayer(ChunkManager* manager) {
    // Player's current chunk
    int px = manager->playerChunk.x;
    int py = manager->playerChunk.y;
    
    // Use a wider radius when player is near edge
    int effectiveRadius = manager->loadRadius + 1;  // Always load one extra chunk out
    
    // Calculate chunk loading bounds with expanded radius
    int startX = px - effectiveRadius;
    int endX = px + effectiveRadius;
    int startY = py - effectiveRadius;
    int endY = py + effectiveRadius;

    printf("\nChunk loading debug:\n");
    printf("Player chunk: (%d,%d)\n", px, py);
    printf("Loading radius: %d (effective: %d)\n", manager->loadRadius, effectiveRadius);
    printf("Chunk area being loaded:\n");
    
    // Print chunk grid visualization
    for (int y = startY; y <= endY; y++) {
        printf("  ");  // Indent
        for (int x = startX; x <= endX; x++) {
            if (x == px && y == py) {
                printf("P "); // Player's chunk
            } else {
                bool isLoaded = false;
                for (int i = 0; i < manager->numLoadedChunks; i++) {
                    if (manager->chunkCoords[i].x == x && manager->chunkCoords[i].y == y) {
                        isLoaded = true;
                        break;
                    }
                }
                printf("%c ", isLoaded ? '#' : '.');  // # for loaded, . for not loaded
            }
        }
        printf("\n");
    }
    printf("\nP = Player's chunk, # = Loaded chunk, . = Unloaded chunk\n");

    // 1) Mark which chunks we want to keep
    bool keepChunk[MAX_LOADED_CHUNKS];
    for (int i = 0; i < manager->numLoadedChunks; i++) {
        keepChunk[i] = false;
        
        // Keep chunks in radius regardless of grid bounds
        int cx = manager->chunkCoords[i].x;
        int cy = manager->chunkCoords[i].y;
        if (cx >= startX && cx <= endX && cy >= startY && cy <= endY) {
            keepChunk[i] = true;
        }
    }

    // 2) Unload out-of-range chunks
    for (int i = manager->numLoadedChunks - 1; i >= 0; i--) {
        if (!keepChunk[i]) {
            Chunk* c = manager->chunks[i];
            if (c) {
                // Unload chunk data from grid
                int gridStartX = c->chunkX * CHUNK_SIZE;
                int gridStartY = c->chunkY * CHUNK_SIZE;
                
                for (int yy = 0; yy < CHUNK_SIZE; yy++) {
                    for (int xx = 0; xx < CHUNK_SIZE; xx++) {
                        int gx = gridStartX + xx;
                        int gy = gridStartY + yy;
                        if (gx >= 0 && gx < GRID_SIZE && gy >= 0 && gy < GRID_SIZE) {
                            grid[gy][gx].terrainType = TERRAIN_UNLOADED;
                            grid[gy][gx].isWalkable = false;
                        }
                    }
                }
                free(c);
            }

            // Remove from manager arrays
            if (i < manager->numLoadedChunks - 1) {
                manager->chunks[i] = manager->chunks[manager->numLoadedChunks - 1];
                manager->chunkCoords[i] = manager->chunkCoords[manager->numLoadedChunks - 1];
            }
            manager->chunks[manager->numLoadedChunks - 1] = NULL;
            manager->numLoadedChunks--;
        }
    }

    // 3) Load new chunks
    for (int cy = startY; cy <= endY; cy++) {
        for (int cx = startX; cx <= endX; cx++) {
            // Check if this chunk would be at least partially in valid grid space
            bool chunkInGrid = false;
            int gridStartX = cx * CHUNK_SIZE;
            int gridStartY = cy * CHUNK_SIZE;
            
            // Check if any corner of the chunk is in bounds
            if ((gridStartX >= 0 && gridStartX < GRID_SIZE && 
                 gridStartY >= 0 && gridStartY < GRID_SIZE) ||
                (gridStartX + CHUNK_SIZE > 0 && gridStartX + CHUNK_SIZE <= GRID_SIZE &&
                 gridStartY + CHUNK_SIZE > 0 && gridStartY + CHUNK_SIZE <= GRID_SIZE)) {
                chunkInGrid = true;
            }

            if (!chunkInGrid) continue;

            // Check if already loaded
            bool alreadyLoaded = false;
            for (int i = 0; i < manager->numLoadedChunks; i++) {
                if (manager->chunkCoords[i].x == cx && manager->chunkCoords[i].y == cy) {
                    alreadyLoaded = true;
                    break;
                }
            }
            
            if (!alreadyLoaded && manager->numLoadedChunks < MAX_LOADED_CHUNKS) {
                printf("Loading new chunk at (%d,%d)\n", cx, cy);
                
                Chunk* newChunk = (Chunk*)malloc(sizeof(Chunk));
                if (!newChunk) {
                    printf("Failed to allocate memory for chunk (%d,%d)\n", cx, cy);
                    continue;
                }
                
                initializeChunk(newChunk, cx, cy);

                if (loadedMapData) {
                    loadMapChunk(loadedMapData, cx, cy, newChunk);
                }

                // Add to manager
                int idx = manager->numLoadedChunks;
                manager->chunks[idx] = newChunk;
                manager->chunkCoords[idx].x = cx;
                manager->chunkCoords[idx].y = cy;
                manager->numLoadedChunks++;

                writeChunkToGrid(newChunk);
            }
        }
    }

    printf("\nFinal chunks loaded: %d (effective radius = %d). Player at chunk: (%d, %d)\n",
           manager->numLoadedChunks, effectiveRadius, px, py);
}