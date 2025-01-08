#ifndef GRID_H
#define GRID_H

#include <stdbool.h>
#include <stdint.h>
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

// Bit manipulation helpers for GridCell

// Set the `isWalkable` bit (Bit 4) in `flags` (1 if `val`, 0 otherwise)
#define GRIDCELL_SET_WALKABLE(cell, val) \
    (cell).flags = ((cell).flags & ~0x10) | ((val) ? 0x10 : 0)

// Check if the `isWalkable` bit (Bit 4) in `flags` is set (nonzero if walkable)
#define GRIDCELL_IS_WALKABLE(cell) \
    ((cell).flags & 0x10)

// Set the `structureOrientation` (Bits 0–3) in `flags` to `val` (masked to 4 bits)
#define GRIDCELL_SET_ORIENTATION(cell, val) \
    (cell).flags = ((cell).flags & ~0x0F) | ((val) & 0x0F)

// Get the `structureOrientation` (Bits 0–3) from `flags` (value 0–15)
#define GRIDCELL_GET_ORIENTATION(cell) \
    ((cell).flags & 0x0F)

typedef struct {
    uint8_t flags;          // Bits 0-3: structureOrientation (N/S/E/W)
                           // Bit 4: isWalkable
                           // Bits 5-7: reserved
    uint8_t terrainType;    // TerrainType enum
    uint8_t structureType;  // StructureType enum
    uint8_t biomeType;      // BiomeType enum
    float wallTexX;         // Keep presentation data together
    float wallTexY;         // as per your request
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