#include "asciiMap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
char* loadedMapData = NULL;  // Add at top of file with other globals

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
    // Only log when chunkX and chunkY are divisible by 5
    if (chunkX % 5 == 0 && chunkY % 5 == 0) {
        printf("\nLoading chunk (%d,%d):\n", chunkX, chunkY);
        printf("Chunk starting at map index: %d\n", chunkY * CHUNK_SIZE * GRID_SIZE + chunkX * CHUNK_SIZE);
        
        int startIndex = (chunkY * CHUNK_SIZE * GRID_SIZE) + (chunkX * CHUNK_SIZE);
        printf("First char in chunk: %c\n", mapData[startIndex]);
    }

    printf("\nLoading chunk (%d,%d) - Raw map data:\n", chunkX, chunkY);
    for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            int mapX = chunkX * CHUNK_SIZE + x;
            int mapY = chunkY * CHUNK_SIZE + y;
            int mapIndex = mapY * GRID_SIZE + mapX;
            printf("%c", mapData[mapIndex]);

            // Initialize the entire cell to 0
            chunk->cells[y][x].flags = 0;
            chunk->cells[y][x].terrainType = 0;
            chunk->cells[y][x].structureType = 0;
            chunk->cells[y][x].biomeType = 0;
            chunk->cells[y][x].wallTexX = 0.0f;
            chunk->cells[y][x].wallTexY = 0.0f;

            char terrainChar = mapData[mapIndex];
            TerrainType terrain = charToTerrain(terrainChar);
            
            // Validate terrain value
            if ((uint8_t)terrain > UINT8_MAX) {
                printf("ERROR: Invalid terrain value detected. Using TERRAIN_GRASS as fallback.\n");
                terrain = TERRAIN_GRASS;
            }

            chunk->cells[y][x].terrainType = (uint8_t)terrain;
            GRIDCELL_SET_WALKABLE(chunk->cells[y][x], (terrain != TERRAIN_WATER));
            GRIDCELL_SET_ORIENTATION(chunk->cells[y][x], 0);
            chunk->cells[y][x].flags &= ~0xE0;  // Clear reserved bits

            uint8_t biomeValue;
            switch (terrain) {
                case TERRAIN_WATER:
                    biomeValue = (uint8_t)BIOME_OCEAN;
                    break;
                case TERRAIN_SAND:
                    biomeValue = (uint8_t)BIOME_BEACH;
                    break;
                case TERRAIN_GRASS:
                    biomeValue = (uint8_t)BIOME_PLAINS;
                    break;
                case TERRAIN_DIRT:
                    biomeValue = (uint8_t)BIOME_FOREST;
                    break;
                case TERRAIN_STONE:
                    biomeValue = (uint8_t)BIOME_MOUNTAINS;
                    break;
                default:
                    biomeValue = (uint8_t)BIOME_PLAINS;
                    break;
            }

            // Validate biome value
            if (biomeValue > UINT8_MAX) {
                printf("ERROR: Invalid biome value detected. Using BIOME_PLAINS as fallback.\n");
                biomeValue = (uint8_t)BIOME_PLAINS;
            }
            chunk->cells[y][x].biomeType = biomeValue;
        }

        if (chunkX % 5 == 0 && chunkY % 5 == 0) {
            printf("Chunk row %d: ", y);
            for (int x = 0; x < CHUNK_SIZE; x++) {
                printf("%c ", terrainToChar((TerrainType)chunk->cells[y][x].terrainType));
            }
            printf("\n");
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