// grid.c

#include "grid.h"
#include <stdlib.h>
#include <stdio.h>
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
    // Nothing to clean up for now
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

/*
 * generateTerrain
 *
 * Generates the terrain for the grid using Perlin noise and biome data.
 */
void generateTerrain() {
    float randomOffset = ((float)rand() / RAND_MAX) * 0.2f - 0.1f;

    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            float biomeValue = perlinNoise(x * 0.1f, y * 0.1f, 0.5f, 4) + randomOffset;
            float heightValue = perlinNoise(x * 0.2f, y * 0.2f, 0.5f, 4) + randomOffset;

            biomeValue = biomeValue < 0 ? 0 : (biomeValue > 1 ? 1 : biomeValue);
            heightValue = heightValue < 0 ? 0 : (heightValue > 1 ? 1 : heightValue);

            BiomeType biome;
            if (biomeValue < 0.1f) biome = BIOME_OCEAN;
            else if (biomeValue < 0.3f) biome = BIOME_BEACH;
            else if (biomeValue < 0.5f) biome = BIOME_PLAINS;
            else if (biomeValue < 0.7f) biome = BIOME_FOREST;
            else if (biomeValue < 0.8f) biome = BIOME_DESERT;
            else biome = BIOME_MOUNTAINS;

            grid[y][x].biomeType = biome;

            if (heightValue > biomeData[biome].heightThresholds[0]) {
                grid[y][x].terrainType = biomeData[biome].terrainTypes[0];
            } else if (heightValue > biomeData[biome].heightThresholds[1]) {
                grid[y][x].terrainType = biomeData[biome].terrainTypes[1];
            } else {
                grid[y][x].terrainType = biomeData[biome].terrainTypes[2];
            }

            grid[y][x].isWalkable = (grid[y][x].terrainType != TERRAIN_WATER && 
                                    grid[y][x].terrainType != TERRAIN_GRASS);
        }
    }

    // Determine the number of water tiles needed for 10% coverage
    int totalTiles = GRID_SIZE * GRID_SIZE;
    int targetWaterTiles = totalTiles * 0.10;

    // Generate water clusters
    int waterTiles = 0;
    while (waterTiles < targetWaterTiles) {
        // Randomly decide the size of the cluster (between 3 and 7)
        int clusterSize = 3 + rand() % 5;

        // Randomly choose the starting point for the cluster
        int startX = rand() % GRID_SIZE;
        int startY = rand() % GRID_SIZE;

        for (int i = 0; i < clusterSize; i++) {
            // Randomly offset from the starting point within a small radius
            int offsetX = (rand() % 3) - 1; // -1, 0, or 1
            int offsetY = (rand() % 3) - 1; // -1, 0, or 1

            int x = startX + offsetX;
            int y = startY + offsetY;

            // Ensure the new coordinates are within bounds
            if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE) {
                // Only convert to water if it is not already water
                if (grid[y][x].terrainType != TERRAIN_WATER) {
                    grid[y][x].terrainType = TERRAIN_WATER;
                    grid[y][x].isWalkable = false;
                    waterTiles++;

                    // Stop if we have reached the target number of water tiles
                    if (waterTiles >= targetWaterTiles) {
                        break;
                    }
                }
            }
        }
    }

    // Ensure the center of the map is walkable for player spawn
    int centerX = GRID_SIZE / 2;
    int centerY = GRID_SIZE / 2;
    for (int y = centerY - 1; y <= centerY + 1; y++) {
        for (int x = centerX - 1; x <= centerX + 1; x++) {
            if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE) {
                grid[y][x].isWalkable = true;
                if (grid[y][x].terrainType == TERRAIN_WATER || grid[y][x].terrainType == TERRAIN_UNWALKABLE) {
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
