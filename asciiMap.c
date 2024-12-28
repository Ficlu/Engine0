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

    // Allocate memory for the entire ASCII map
    loadedMapData = (char*)malloc(GRID_SIZE * GRID_SIZE * sizeof(char));
    if (!loadedMapData) {
        printf("Failed to allocate memory for ASCII map\n");
        fclose(file);
        return NULL;
    }

    // Read the map file line-by-line into loadedMapData
    int index = 0;
    char line[GRID_SIZE + 2];  // +2 for newline + null terminator
    while (fgets(line, sizeof(line), file) && index < GRID_SIZE * GRID_SIZE) {
        line[strcspn(line, "\r\n")] = '\0';  // remove newline
        strncpy(&loadedMapData[index], line, GRID_SIZE);
        index += GRID_SIZE;
    }
    fclose(file);

    // ==========================
    // Remove any chunk-creation loop here!
    // ==========================

    printf("ASCII map successfully loaded into memory.\n");
    return loadedMapData;
}
void loadMapChunk(const char* mapData, int chunkX, int chunkY, Chunk* chunk) {
    // Only log when chunkX and chunkY are divisible by 5
    if (chunkX % 5 == 0 && chunkY % 5 == 0) {
        printf("\nLoading chunk (%d,%d):\n", chunkX, chunkY);
        printf("Chunk starting at map index: %d\n", chunkY * CHUNK_SIZE * GRID_SIZE + chunkX * CHUNK_SIZE);
        
        // Add this debug print to see the first character of the chunk
        int startIndex = (chunkY * CHUNK_SIZE * GRID_SIZE) + (chunkX * CHUNK_SIZE);
        printf("First char in chunk: %c\n", mapData[startIndex]);
    }
    // Debug: Print the raw map data for this chunk
    printf("\nLoading chunk (%d,%d) - Raw map data:\n", chunkX, chunkY);
    for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            int mapX = chunkX * CHUNK_SIZE + x;
            int mapY = chunkY * CHUNK_SIZE + y;
            int mapIndex = mapY * GRID_SIZE + mapX;
            printf("%c", mapData[mapIndex]);

            char terrainChar = mapData[mapIndex];
            TerrainType terrain = charToTerrain(terrainChar);

            chunk->cells[y][x].terrainType = terrain;
            chunk->cells[y][x].isWalkable = (terrain != TERRAIN_WATER);

            switch (terrain) {
                case TERRAIN_WATER: chunk->cells[y][x].biomeType = BIOME_OCEAN; break;
                case TERRAIN_SAND: chunk->cells[y][x].biomeType = BIOME_BEACH; break;
                case TERRAIN_GRASS: chunk->cells[y][x].biomeType = BIOME_PLAINS; break;
                case TERRAIN_DIRT: chunk->cells[y][x].biomeType = BIOME_FOREST; break;
                case TERRAIN_STONE: chunk->cells[y][x].biomeType = BIOME_MOUNTAINS; break;
                default: chunk->cells[y][x].biomeType = BIOME_PLAINS; break;
            }
        }

        // Print each row of the chunk as we load it, but only if chunkX and chunkY are divisible by 5
        if (chunkX % 5 == 0 && chunkY % 5 == 0) {
            printf("Chunk row %d: ", y);
            for (int x = 0; x < CHUNK_SIZE; x++) {
                printf("%c ", terrainToChar(chunk->cells[y][x].terrainType));
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
        case CHAR_WATER: return TERRAIN_WATER; // e.g. '0' or '~'
        case CHAR_SAND:  return TERRAIN_SAND;  // e.g. '.'
        case CHAR_GRASS: return TERRAIN_GRASS; // e.g. '3'
        case CHAR_STONE: return TERRAIN_STONE; // e.g. '4'
        default:         return TERRAIN_GRASS; // fallback
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
            fputc(terrainToChar(grid[y][x].terrainType), file);
        }
        fputc('\n', file);
    }

    fclose(file);
    printf("Grid saved as ASCII map to: %s\n", filename);
}