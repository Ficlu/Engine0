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