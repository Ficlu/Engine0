#ifndef GRID_H
#define GRID_H

#include <stdbool.h>

#define GRID_SIZE 40
#define CHUNK_SIZE 8  // 8x8 chunks
#define NUM_CHUNKS (GRID_SIZE / CHUNK_SIZE)
#define MAX_LOADED_CHUNKS 25  // 5x5 area around player

typedef enum {
    TERRAIN_WATER,
    TERRAIN_SAND,
    TERRAIN_GRASS,
    TERRAIN_DIRT, 
    TERRAIN_STONE,
    TERRAIN_UNWALKABLE,
    TERRAIN_UNLOADED  // Represents "not currently loaded"
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
    bool hasWall;  // New field
    float wallTexX;
    float wallTexY;
    bool isDoorOpen;  // New field

} GridCell;


typedef struct {
    int x;
    int y;
} ChunkCoord;

typedef struct {
    GridCell cells[CHUNK_SIZE][CHUNK_SIZE];
    int chunkX;
    int chunkY;
    bool isLoaded;
} Chunk;

typedef struct {
    Chunk* chunks[MAX_LOADED_CHUNKS];
    ChunkCoord chunkCoords[MAX_LOADED_CHUNKS];
    ChunkCoord playerChunk;
    int loadRadius;
    int numLoadedChunks;
    GridCell storedChunkData[NUM_CHUNKS][NUM_CHUNKS][CHUNK_SIZE][CHUNK_SIZE];  // New storage array
    bool chunkHasData[NUM_CHUNKS][NUM_CHUNKS];  // Track which chunks have saved data
} ChunkManager;

typedef struct {
    float texX;
    float texY;
} WallTextureCoords;

extern GridCell grid[GRID_SIZE][GRID_SIZE];
extern BiomeData biomeData[BIOME_COUNT];
extern ChunkManager* globalChunkManager;

// Existing functions
void initializeGrid(int size);
void cleanupGrid();
bool isWalkable(int x, int y);
void setGridSize(int size);
bool isValid(int x, int y);
void generateTerrain();
// Add these with the other function declarations in grid.h:
void generateTerrainForChunk(Chunk* chunk);
void initializeChunk(Chunk* chunk, int chunkX, int chunkY);
void processChunk(Chunk* chunk);
void writeChunkToGrid(const Chunk* chunk);
void debugPrintGridSection(int startX, int startY, int width, int height);

// New chunk management functions
void initChunkManager(ChunkManager* manager, int loadRadius);
void cleanupChunkManager(ChunkManager* manager);
ChunkCoord getChunkFromWorldPos(float worldX, float worldY);
bool isPositionInLoadedChunk(float worldX, float worldY);

bool isChunkLoaded(ChunkManager* manager, int chunkX, int chunkY);
void updatePlayerChunk(ChunkManager* manager, float playerX, float playerY);
void loadChunksAroundPlayer(ChunkManager* manager);
Chunk* getChunk(ChunkManager* manager, int chunkX, int chunkY);
#endif // GRID_H