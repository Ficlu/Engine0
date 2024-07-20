#include "grid.h"
#include <stdlib.h>
#include <stdio.h>

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

void initializeGrid(int size) {
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            grid[y][x].terrainType = TERRAIN_GRASS;
            grid[y][x].biomeType = BIOME_PLAINS;
            grid[y][x].isWalkable = true;
        }
    }
}

void cleanupGrid() {
    // Nothing to clean up for now
}

bool isWalkable(int x, int y) {
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) {
        return false;
    }
    return grid[y][x].isWalkable;
}

bool isValid(int x, int y) {
    return x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE;
}

void setGridSize(int size) {
    // We're using a fixed size grid for now, so this function doesn't do much
    if (size != GRID_SIZE) {
        printf("Warning: Attempted to set grid size to %d, but GRID_SIZE is fixed at %d\n", size, GRID_SIZE);
    }
}

void generateTerrain() {
    float randomOffset = ((float)rand() / RAND_MAX) * 0.2f - 0.1f;  // Random value between -0.1 and 0.1

    // First pass: generate basic terrain
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            float biomeValue = perlinNoise(x * 0.1f, y * 0.1f, 0.5f, 4) + randomOffset;
            float heightValue = perlinNoise(x * 0.2f, y * 0.2f, 0.5f, 4) + randomOffset;

            // Clamp values to [0, 1] range
            biomeValue = biomeValue < 0 ? 0 : (biomeValue > 1 ? 1 : biomeValue);
            heightValue = heightValue < 0 ? 0 : (heightValue > 1 ? 1 : heightValue);

            // Determine biome
            BiomeType biome;
            if (biomeValue < 0.2f) biome = BIOME_OCEAN;
            else if (biomeValue < 0.3f) biome = BIOME_BEACH;
            else if (biomeValue < 0.5f) biome = BIOME_PLAINS;
            else if (biomeValue < 0.7f) biome = BIOME_FOREST;
            else if (biomeValue < 0.8f) biome = BIOME_DESERT;
            else biome = BIOME_MOUNTAINS;

            grid[y][x].biomeType = biome;

            // Determine terrain type based on biome and height
            if (heightValue > biomeData[biome].heightThresholds[0]) {
                grid[y][x].terrainType = biomeData[biome].terrainTypes[0];
            } else if (heightValue > biomeData[biome].heightThresholds[1]) {
                grid[y][x].terrainType = biomeData[biome].terrainTypes[1];
            } else {
                grid[y][x].terrainType = biomeData[biome].terrainTypes[2];
            }

            // Set initial walkability
            grid[y][x].isWalkable = (grid[y][x].terrainType != TERRAIN_WATER);
        }
    }

    // Second pass: randomly make some tiles unwalkable and update their terrain type
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (grid[y][x].isWalkable && ((float)rand() / RAND_MAX) < UNWALKABLE_PROBABILITY) {
                grid[y][x].isWalkable = false;
                grid[y][x].terrainType = TERRAIN_UNWALKABLE;
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
float noise(int x, int y) {
    int n = x + y * 57;
    n = (n << 13) ^ n;
    return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589 + rand()) & 0x7fffffff) / 1073741824.0f);
}

float smoothNoise(int x, int y) {
    float corners = (noise(x-1, y-1) + noise(x+1, y-1) + noise(x-1, y+1) + noise(x+1, y+1)) / 16.0f;
    float sides = (noise(x-1, y) + noise(x+1, y) + noise(x, y-1) + noise(x, y+1)) / 8.0f;
    float center = noise(x, y) / 4.0f;
    return corners + sides + center;
}

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
