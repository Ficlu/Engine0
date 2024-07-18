#ifndef GRID_H
#define GRID_H

#include <stdbool.h>

#define GRID_SIZE 40

typedef enum {
    TERRAIN_WATER,
    TERRAIN_SAND,
    TERRAIN_GRASS,
    TERRAIN_DIRT,
    TERRAIN_STONE
} TerrainType;

typedef enum {
    BIOME_OCEAN,
    BIOME_BEACH,
    BIOME_PLAINS,
    BIOME_FOREST,
    BIOME_DESERT,
    BIOME_MOUNTAINS,
    BIOME_COUNT
} BiomeType;

typedef struct {
    TerrainType terrainTypes[3]; // Top, middle, bottom layers
    float heightThresholds[2]; // Thresholds between layers
} BiomeData;

typedef struct {
    TerrainType terrainType;
    BiomeType biomeType;
    bool isWalkable;
} GridCell;

extern GridCell grid[GRID_SIZE][GRID_SIZE];
extern BiomeData biomeData[BIOME_COUNT];

void initializeGrid(int size);
void cleanupGrid();
bool isWalkable(int x, int y);
void setGridSize(int size);
bool isValid(int x, int y);

#endif // GRID_H