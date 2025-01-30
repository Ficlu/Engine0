#include "asciiMap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
char* loadedMapData = NULL;  // Add at top of file with other globals

static uint16_t wang_hash(uint32_t seed) {
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed % 4;  // Returns 0-3
}

static uint16_t get_tile_variation(int x, int y) {
    // Combine coordinates into a single seed
    uint32_t seed = (uint32_t)(x * 374761393 + y * 668265263);
    // Add a large prime for more randomness
    seed ^= 0x6eed0e9d;
    return wang_hash(seed);
}

static uint16_t get_tile_rotation(int x, int y) {
    // Use different prime multipliers for rotation
    uint32_t seed = (uint32_t)(x * 487198191 + y * 286265417);
    // Different prime than variation
    seed ^= 0x43e9b4af;
    return wang_hash(seed);
}

char* loadASCIIMap(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Failed to open file: %s\n", filename);
        return NULL;
    }

    // Free any previously loaded map data
    if (loadedMapData) {
        free(loadedMapData);
        loadedMapData = NULL;
    }

    // Just load the raw data into memory - don't process chunks yet
    loadedMapData = (char*)malloc(GRID_SIZE * GRID_SIZE * sizeof(char));
    if (!loadedMapData) {
        printf("Failed to allocate memory for ASCII map\n");
        fclose(file);
        return NULL;
    }

    int index = 0;
    char line[GRID_SIZE + 2];  // +2 for newline + null terminator
    while (fgets(line, sizeof(line), file) && index < GRID_SIZE * GRID_SIZE) {
        line[strcspn(line, "\r\n")] = '\0';  // remove newline
        strncpy(&loadedMapData[index], line, GRID_SIZE);
        index += GRID_SIZE;
    }
    fclose(file);

    printf("ASCII map data loaded into memory.\n");
    return loadedMapData;
}
void loadMapChunk(const char* mapData, int chunkX, int chunkY, Chunk* chunk) {
    for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            int mapX = chunkX * CHUNK_SIZE + x;
            int mapY = chunkY * CHUNK_SIZE + y;
            int mapIndex = mapY * GRID_SIZE + mapX;
            
            char terrainChar = mapData[mapIndex];

            // Preserve existing structure data by not resetting it
            // Only clear terrain-related flags, keeping structure bits
            uint16_t existingFlags = chunk->cells[y][x].flags;
            uint16_t structureFlags = existingFlags & STRUCTURE_PRESERVE_MASK;
            chunk->cells[y][x].flags = structureFlags;  // Keep structure flags, clear terrain flags

            uint8_t assignedType;
            if (terrainChar == CHAR_GRASS) {
                assignedType = TERRAIN_GRASS;
                uint16_t variation = get_tile_variation(mapX, mapY);
                GRIDCELL_SET_TERRAIN_VARIATION(chunk->cells[y][x], variation);
            }
            else if (terrainChar == CHAR_SAND) {
                assignedType = TERRAIN_SAND;
            }
            else if (terrainChar == CHAR_WATER) {
                assignedType = TERRAIN_WATER;
            }
            else if (terrainChar == CHAR_STONE) {
                assignedType = TERRAIN_STONE;
                uint16_t variation = get_tile_variation(mapX, mapY);
                GRIDCELL_SET_TERRAIN_VARIATION(chunk->cells[y][x], variation);
            }
            else {
                assignedType = TERRAIN_GRASS;
                uint16_t variation = get_tile_variation(mapX, mapY);
                GRIDCELL_SET_TERRAIN_VARIATION(chunk->cells[y][x], variation);
            }

            chunk->cells[y][x].terrainType = assignedType;
            uint8_t tileRotation = get_tile_rotation(mapX, mapY);
            GRIDCELL_SET_TERRAIN_ROTATION(chunk->cells[y][x], tileRotation);
            
            // Only update walkable if there's no structure
            if (chunk->cells[y][x].structureType == 0) {
                GRIDCELL_SET_WALKABLE(chunk->cells[y][x], (assignedType != TERRAIN_WATER));
            }
        }
    }
}
void generateTerrainFromASCII(const char* asciiMap) {
    for (int chunkY = 0; chunkY < NUM_CHUNKS; chunkY++) {
        for (int chunkX = 0; chunkX < NUM_CHUNKS; chunkX++) {
            Chunk chunk;
            chunk.chunkX = chunkX;
            chunk.chunkY = chunkY;
            
            loadMapChunk(asciiMap, chunkX, chunkY, &chunk);
            processChunk(&chunk);
            writeChunkToGrid(&chunk);
        }
    }
}

// Keep existing functions unchanged
TerrainType charToTerrain(char c) {
    switch(c) {
        case CHAR_WATER: return (TerrainType)TERRAIN_WATER; // e.g. '0' or '~'
        case CHAR_SAND:  return (TerrainType)TERRAIN_SAND;  // e.g. '.'
        case CHAR_GRASS: return (TerrainType)TERRAIN_GRASS; // e.g. '3'
        case CHAR_STONE: return (TerrainType)TERRAIN_STONE; // e.g. '4'
        default:         return (TerrainType)TERRAIN_GRASS; // fallback
    }
}


char terrainToChar(TerrainType terrain) {
    switch(terrain) {
        case TERRAIN_WATER: return CHAR_WATER;
        case TERRAIN_SAND: return CHAR_SAND;
        case TERRAIN_GRASS: return CHAR_GRASS;
        case TERRAIN_STONE: return CHAR_STONE;
        default: return CHAR_GRASS;
    }
}

// Existing save function remains unchanged
void saveGridAsASCII(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        printf("Failed to open file for writing: %s\n", filename);
        return;
    }

    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            fputc(terrainToChar((TerrainType)grid[y][x].terrainType), file);

        }
        fputc('\n', file);
    }

    fclose(file);
    printf("Grid saved as ASCII map to: %s\n", filename);
}