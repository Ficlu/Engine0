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
            GRIDCELL_SET_WALKABLE(grid[y][x], true);
            grid[y][x].structureType = 0; // Assuming 0 represents no structure
            GRIDCELL_SET_ORIENTATION(grid[y][x], 0);
            grid[y][x].flags &= 0x1F; // Clear reserved bits
            grid[y][x].wallTexX = 0.0f / 3.0f;  // Default wall texture coordinates
            grid[y][x].wallTexY = 1.0f / 4.0f;  // Vertical wall texture (default)
        }
    }
    printf("Grid initialized with size %d x %d\n", size, size);
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
    return GRIDCELL_IS_WALKABLE(grid[y][x]);

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

void writeChunkToGrid(const Chunk* chunk) {
    int startX = chunk->chunkX * CHUNK_SIZE;
    int startY = chunk->chunkY * CHUNK_SIZE;
    
    for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            int gridX = startX + x;
            int gridY = startY + y;
            
            if (gridX < GRID_SIZE && gridY < GRID_SIZE) {
                // Preserve existing structure data
                uint8_t oldStructureType = grid[gridY][gridX].structureType;
                uint8_t oldOrientation = GRIDCELL_GET_ORIENTATION(grid[gridY][gridX]);
                bool wasWalkable = GRIDCELL_IS_WALKABLE(grid[gridY][gridX]);
                float oldTexX = grid[gridY][gridX].wallTexX;
                float oldTexY = grid[gridY][gridX].wallTexY;
                
                // Update the grid cell with new chunk data
                grid[gridY][gridX] = chunk->cells[y][x];
                
                // Restore structure data if it existed
                if (oldStructureType != 0) {
                    grid[gridY][gridX].structureType = oldStructureType;
                    GRIDCELL_SET_ORIENTATION(grid[gridY][gridX], oldOrientation);
                    GRIDCELL_SET_WALKABLE(grid[gridY][gridX], wasWalkable);
                    grid[gridY][gridX].wallTexX = oldTexX;
                    grid[gridY][gridX].wallTexY = oldTexY;
                }
                
                // Ensure reserved bits are clear
                grid[gridY][gridX].flags &= 0x1F;
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
                GRIDCELL_SET_WALKABLE(grid[y][x], true);

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

    // Initialize stored chunk data tracking
    for (int cy = 0; cy < NUM_CHUNKS; cy++) {
        for (int cx = 0; cx < NUM_CHUNKS; cx++) {
            manager->chunkHasData[cy][cx] = false;
            // Initialize stored chunk data
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int x = 0; x < CHUNK_SIZE; x++) {
                    manager->storedChunkData[cy][cx][y][x].terrainType = TERRAIN_GRASS;
                    manager->storedChunkData[cy][cx][y][x].biomeType = BIOME_PLAINS;
                    GRIDCELL_SET_WALKABLE(manager->storedChunkData[cy][cx][y][x], true);
                    manager->storedChunkData[cy][cx][y][x].structureType = 0;
                    GRIDCELL_SET_ORIENTATION(manager->storedChunkData[cy][cx][y][x], 0);
                    manager->storedChunkData[cy][cx][y][x].flags &= 0x1F;
                }
            }
        }
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
            GRIDCELL_SET_WALKABLE(chunk->cells[y][x], true);
            chunk->cells[y][x].structureType = 0;
            GRIDCELL_SET_ORIENTATION(chunk->cells[y][x], 0);
            chunk->cells[y][x].flags &= 0x1F; // Clear reserved bits
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
    
    // Convert to chunk coordinates with bounds checking
    coord.x = (int)floorf(gridX / CHUNK_SIZE);
    coord.y = (int)floorf(gridY / CHUNK_SIZE);
    
    // Clamp to valid range
    coord.x = fmax(0, fmin(coord.x, NUM_CHUNKS - 1));
    coord.y = fmax(0, fmin(coord.y, NUM_CHUNKS - 1));
    
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
    
    // Only trigger chunk loading if player has moved to a different chunk
    if (currentChunk.x != manager->playerChunk.x || 
        currentChunk.y != manager->playerChunk.y) {
        
        printf("Player moved to new chunk: (%d,%d) -> (%d,%d)\n", 
               manager->playerChunk.x, manager->playerChunk.y,
               currentChunk.x, currentChunk.y);
               
        manager->playerChunk = currentChunk;
        loadChunksAroundPlayer(manager);
    }
}

bool isPositionInLoadedChunk(float worldX, float worldY) {
    ChunkCoord coord = getChunkFromWorldPos(worldX, worldY);
    return isChunkLoaded(globalChunkManager, coord.x, coord.y);
}



void loadChunksAroundPlayer(ChunkManager* manager) {
    if (!manager) return;

    int px = manager->playerChunk.x;
    int py = manager->playerChunk.y;
    int effectiveRadius = manager->loadRadius;
    
    printf("\nUnloading chunks outside radius %d of (%d,%d)\n", 
           effectiveRadius, px, py);

    // First: Aggressively unload all chunks outside radius
    for (int i = manager->numLoadedChunks - 1; i >= 0; i--) {
        int dx = abs(manager->chunkCoords[i].x - px);
        int dy = abs(manager->chunkCoords[i].y - py);
        
        if (dx > effectiveRadius || dy > effectiveRadius) {
            printf("Unloading chunk at (%d,%d) - distance (%d,%d) exceeds radius\n",
                   manager->chunkCoords[i].x, manager->chunkCoords[i].y, dx, dy);
            
            // Store chunk data before unloading
            int chunkX = manager->chunkCoords[i].x;
            int chunkY = manager->chunkCoords[i].y;
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int x = 0; x < CHUNK_SIZE; x++) {
                    int gridX = chunkX * CHUNK_SIZE + x;
                    int gridY = chunkY * CHUNK_SIZE + y;
                    if (gridX >= 0 && gridX < GRID_SIZE && 
                        gridY >= 0 && gridY < GRID_SIZE) {
                        // Store the current state
                        manager->storedChunkData[chunkY][chunkX][y][x] = grid[gridY][gridX];
                        // Mark grid as unloaded
                        grid[gridY][gridX].terrainType = TERRAIN_UNLOADED;
                        GRIDCELL_SET_WALKABLE(grid[gridY][gridX], false);

                    }
                }
            }
            manager->chunkHasData[chunkY][chunkX] = true;
            
            // Free and remove the chunk
            free(manager->chunks[i]);
            if (i < manager->numLoadedChunks - 1) {
                manager->chunks[i] = manager->chunks[manager->numLoadedChunks - 1];
                manager->chunkCoords[i] = manager->chunkCoords[manager->numLoadedChunks - 1];
            }
            manager->chunks[manager->numLoadedChunks - 1] = NULL;
            manager->numLoadedChunks--;
        }
    }

    // Then: Load new chunks within radius
    for (int dy = -effectiveRadius; dy <= effectiveRadius; dy++) {
        for (int dx = -effectiveRadius; dx <= effectiveRadius; dx++) {
            int cx = px + dx;
            int cy = py + dy;
            
            // Skip if out of bounds
            if (cx < 0 || cx >= NUM_CHUNKS || cy < 0 || cy >= NUM_CHUNKS) {
                continue;
            }

            // Check if already loaded
            bool alreadyLoaded = false;
            for (int i = 0; i < manager->numLoadedChunks; i++) {
                if (manager->chunkCoords[i].x == cx && 
                    manager->chunkCoords[i].y == cy) {
                    alreadyLoaded = true;
                    break;
                }
            }

            if (!alreadyLoaded && manager->numLoadedChunks < MAX_LOADED_CHUNKS) {
                printf("Loading new chunk at (%d,%d)\n", cx, cy);
                
                Chunk* newChunk = (Chunk*)malloc(sizeof(Chunk));
                if (!newChunk) continue;

                initializeChunk(newChunk, cx, cy);
                
                if (manager->chunkHasData[cy][cx]) {
                    // Restore saved chunk data
                    for (int y = 0; y < CHUNK_SIZE; y++) {
                        for (int x = 0; x < CHUNK_SIZE; x++) {
                            newChunk->cells[y][x] = manager->storedChunkData[cy][cx][y][x];
                        }
                    }
                } else if (loadedMapData) {
                    loadMapChunk(loadedMapData, cx, cy, newChunk);
                }

                manager->chunks[manager->numLoadedChunks] = newChunk;
                manager->chunkCoords[manager->numLoadedChunks] = (ChunkCoord){cx, cy};
                manager->numLoadedChunks++;
                
                writeChunkToGrid(newChunk);
            }
        }
    }
}