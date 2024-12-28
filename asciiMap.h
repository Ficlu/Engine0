#ifndef ASCII_MAP_H
#define ASCII_MAP_H

#include "grid.h"

#define CHAR_STONE '1'
#define CHAR_SAND '2'
#define CHAR_WATER '3'
#define CHAR_GRASS '4'

// Modified function to load chunks from an ASCII map file
char* loadASCIIMap(const char* filename);

// New function to load a specific chunk from the map data
void loadMapChunk(const char* mapData, int chunkX, int chunkY, Chunk* chunk);

// Existing functions remain unchanged
void generateTerrainFromASCII(const char* asciiMap);
TerrainType charToTerrain(char c);
char terrainToChar(TerrainType terrain);
void saveGridAsASCII(const char* filename);

#endif // ASCII_MAP_H