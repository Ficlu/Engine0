#ifdef __SSE4_1__
#define USE_SIMD
#endif

#ifdef USE_SIMD
#include <immintrin.h>
#endif
#ifndef GRID_H
#define GRID_H

#include <stdbool.h>
#include <stdint.h>

// Grid and chunk size definitions
#define GRID_SIZE 40
#define CHUNK_SIZE 8  // 8x8 chunks
#define NUM_CHUNKS (GRID_SIZE / CHUNK_SIZE)
#define MAX_LOADED_CHUNKS 25  // 5x5 area around player

// Grid Cell Flag Bit Layout (16-bit)
// --------------------------------
#define STRUCTURE_ORIENTATION_MASK 0x000F  // Bits 0-3:   Structure orientation (16 possible orientations)
#define WALKABLE_MASK             0x0010  // Bit 4:      Walkable flag (0=blocked, 1=walkable)
#define TERRAIN_ROTATION_MASK     0x0060  // Bits 5-6:   Terrain rotation (0-3, 90-degree increments)
#define STRUCTURE_ROTATION_MASK   0x0080  // Bit 7:      Structure rotation flag
#define TERRAIN_VARIATION_MASK    0x0300  // Bits 8-9:   Terrain variation (0-3)
#define STRUCTURE_FLAGS_MASK      0xFC00  // Bits 10-15: Reserved for structure-specific data

// Combined masks for operations
#define TERRAIN_MASK (TERRAIN_ROTATION_MASK | TERRAIN_VARIATION_MASK)
#define STRUCTURE_PRESERVE_MASK (STRUCTURE_ORIENTATION_MASK | WALKABLE_MASK | STRUCTURE_ROTATION_MASK | STRUCTURE_FLAGS_MASK)

// Grid cell manipulation macros
#define GRIDCELL_SET_ORIENTATION(cell, val) \
    (cell).flags = ((cell).flags & ~STRUCTURE_ORIENTATION_MASK) | ((val) & 0x0F)

#define GRIDCELL_GET_ORIENTATION(cell) \
    ((cell).flags & STRUCTURE_ORIENTATION_MASK)

#define GRIDCELL_SET_WALKABLE(cell, val) \
    (cell).flags = ((cell).flags & ~WALKABLE_MASK) | ((val) ? WALKABLE_MASK : 0)

#define GRIDCELL_IS_WALKABLE(cell) \
    ((cell).flags & WALKABLE_MASK)

#define GRIDCELL_SET_TERRAIN_ROTATION(cell, rot) \
    (cell).flags = ((cell).flags & ~TERRAIN_ROTATION_MASK) | (((rot) << 5) & TERRAIN_ROTATION_MASK)

#define GRIDCELL_GET_TERRAIN_ROTATION(cell) \
    ((cell).flags & TERRAIN_ROTATION_MASK) >> 5

#define GRIDCELL_SET_STRUCTURE_ROTATION(cell, rot) \
    (cell).flags = ((cell).flags & ~STRUCTURE_ROTATION_MASK) | (((rot) << 7) & STRUCTURE_ROTATION_MASK)

#define GRIDCELL_GET_STRUCTURE_ROTATION(cell) \
    ((cell).flags & STRUCTURE_ROTATION_MASK) >> 7

#define GRIDCELL_SET_TERRAIN_VARIATION(cell, var) \
    (cell).flags = ((cell).flags & ~TERRAIN_VARIATION_MASK) | (((var) << 8) & TERRAIN_VARIATION_MASK)

#define GRIDCELL_GET_TERRAIN_VARIATION(cell) \
    ((cell).flags & TERRAIN_VARIATION_MASK) >> 8

// Type definitions
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
    uint16_t flags;          // Bit field for various flags (see bit layout above)
    uint8_t terrainType;     // TerrainType enum
    uint8_t structureType;   // StructureType enum
    uint8_t biomeType;       // BiomeType enum
    uint8_t materialType;    // MaterialType enum
    float wallTexX;          // Texture coordinates
    float wallTexY;
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
    GridCell storedChunkData[NUM_CHUNKS][NUM_CHUNKS][CHUNK_SIZE][CHUNK_SIZE];
    bool chunkHasData[NUM_CHUNKS][NUM_CHUNKS];
} ChunkManager;

// External declarations
extern GridCell grid[GRID_SIZE][GRID_SIZE];
extern BiomeData biomeData[BIOME_COUNT];
extern ChunkManager* globalChunkManager;

// Function declarations
void initializeGrid(int size);
void cleanupGrid(void);
bool isWalkable(int x, int y);
void setGridSize(int size);
bool isValid(int x, int y);
void generateTerrain(void);
void generateTerrainForChunk(Chunk* chunk);
void initializeChunk(Chunk* chunk, int chunkX, int chunkY);
void processChunk(Chunk* chunk);
void writeChunkToGrid(const Chunk* chunk);
void debugPrintGridSection(int startX, int startY, int width, int height);

// Chunk management functions
void initChunkManager(ChunkManager* manager, int loadRadius);
void cleanupChunkManager(ChunkManager* manager);
ChunkCoord getChunkFromWorldPos(float worldX, float worldY);
bool isPositionInLoadedChunk(float worldX, float worldY);
bool isChunkLoaded(ChunkManager* manager, int chunkX, int chunkY);
void updatePlayerChunk(ChunkManager* manager, float playerX, float playerY);
void loadChunksAroundPlayer(ChunkManager* manager);
Chunk* getChunk(ChunkManager* manager, int chunkX, int chunkY);

#endif // GRID_H