#ifndef GRID_H
#define GRID_H

#include <stdbool.h>

#define GRID_SIZE 40
#define CHUNK_SIZE 8  // 8x8 chunks
#define NUM_CHUNKS (GRID_SIZE / CHUNK_SIZE)

typedef enum {
    TERRAIN_WATER,
    TERRAIN_SAND,
    TERRAIN_GRASS,
    TERRAIN_DIRT, 
    TERRAIN_STONE,
    TERRAIN_UNWALKABLE
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
    TerrainType terrainTypes[3];
    float heightThresholds[2];
} BiomeData;

typedef struct {
    TerrainType terrainType;
    BiomeType biomeType;
    bool isWalkable;
} GridCell;

typedef struct {
    GridCell cells[CHUNK_SIZE][CHUNK_SIZE];
    int chunkX;
    int chunkY;
} Chunk;

extern GridCell grid[GRID_SIZE][GRID_SIZE];
extern BiomeData biomeData[BIOME_COUNT];

void initializeGrid(int size);
void cleanupGrid();
bool isWalkable(int x, int y);
void setGridSize(int size);
bool isValid(int x, int y);
void generateTerrain();
void processChunk(Chunk* chunk);
void writeChunkToGrid(const Chunk* chunk);
void debugPrintGridSection(int startX, int startY, int width, int height);
#endif // GRID_H