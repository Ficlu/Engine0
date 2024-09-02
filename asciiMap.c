// ascii_map.c

#include "asciiMap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    char line[GRID_SIZE + 2]; // +2 for the newline and null-terminator
    int index = 0;

    while (fgets(line, sizeof(line), file)) {
        // Strip newline character if present
        line[strcspn(line, "\r\n")] = '\0';

        // Copy the line into the asciiMap
        strncpy(&asciiMap[index], line, GRID_SIZE);
        index += GRID_SIZE;

        if (index >= GRID_SIZE * GRID_SIZE) {
            break;
        }
    }

    fclose(file);
    return asciiMap;
}
void generateTerrainFromASCII(const char* asciiMap) {
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            char c = asciiMap[y * GRID_SIZE + x];
            grid[y][x].terrainType = charToTerrain(c);
            grid[y][x].isWalkable = (c != CHAR_WATER);
            
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
        case CHAR_STONE: return TERRAIN_STONE;
        default: return TERRAIN_GRASS; // Default to grass for unknown characters
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