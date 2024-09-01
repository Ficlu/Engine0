// ascii_map.c

#include "asciiMap.h"
#include <stdio.h>
#include <stdlib.h>

char* loadASCIIMap(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Failed to open file: %s\n", filename);
        return NULL;
    }

    char* asciiMap = (char*)malloc(GRID_SIZE * GRID_SIZE * sizeof(char));
    if (!asciiMap) {
        printf("Failed to allocate memory for ASCII map\n");
        fclose(file);
        return NULL;
    }

    size_t read = fread(asciiMap, sizeof(char), GRID_SIZE * GRID_SIZE, file);
    if (read != GRID_SIZE * GRID_SIZE) {
        printf("Failed to read complete ASCII map\n");
        free(asciiMap);
        fclose(file);
        return NULL;
    }

    fclose(file);
    return asciiMap;
}

void generateTerrainFromASCII(const char* asciiMap) {
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            char c = asciiMap[y * GRID_SIZE + x];
            grid[y][x].terrainType = charToTerrain(c);
            grid[y][x].isWalkable = (c != CHAR_UNWALKABLE && c != CHAR_WATER);
            
            // Set biome based on terrain (you may want to adjust this logic)
            switch(grid[y][x].terrainType) {
                case TERRAIN_WATER: grid[y][x].biomeType = BIOME_OCEAN; break;
                case TERRAIN_SAND: grid[y][x].biomeType = BIOME_BEACH; break;
                case TERRAIN_GRASS: grid[y][x].biomeType = BIOME_PLAINS; break;
                case TERRAIN_DIRT: grid[y][x].biomeType = BIOME_FOREST; break;
                case TERRAIN_STONE: grid[y][x].biomeType = BIOME_MOUNTAINS; break;
                default: grid[y][x].biomeType = BIOME_PLAINS; break;
            }
        }
    }
}

TerrainType charToTerrain(char c) {
    switch(c) {
        case CHAR_WATER: return TERRAIN_WATER;
        case CHAR_SAND: return TERRAIN_SAND;
        case CHAR_GRASS: return TERRAIN_GRASS;
        case CHAR_DIRT: return TERRAIN_DIRT;
        case CHAR_STONE: return TERRAIN_STONE;
        case CHAR_UNWALKABLE: return TERRAIN_UNWALKABLE;
        default: return TERRAIN_GRASS; // Default to grass for unknown characters
    }
}

char terrainToChar(TerrainType terrain) {
    switch(terrain) {
        case TERRAIN_WATER: return CHAR_WATER;
        case TERRAIN_SAND: return CHAR_SAND;
        case TERRAIN_GRASS: return CHAR_GRASS;
        case TERRAIN_DIRT: return CHAR_DIRT;
        case TERRAIN_STONE: return CHAR_STONE;
        case TERRAIN_UNWALKABLE: return CHAR_UNWALKABLE;
        default: return CHAR_GRASS;
    }
}

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
        fputc('\n', file);  // Add a newline after each row for readability
    }

    fclose(file);
    printf("Grid saved as ASCII map to: %s\n", filename);
}