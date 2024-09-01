// ascii_map.h

#ifndef ASCII_MAP_H
#define ASCII_MAP_H

#include "grid.h"

#define CHAR_WATER '1'
#define CHAR_SAND '2'
#define CHAR_GRASS '3'
#define CHAR_DIRT '4'
#define CHAR_STONE '5'
#define CHAR_UNWALKABLE '.'

// Function to load an ASCII map from a file
char* loadASCIIMap(const char* filename);

// Function to generate terrain from an ASCII map
void generateTerrainFromASCII(const char* asciiMap);

// Function to convert a character to a terrain type
TerrainType charToTerrain(char c);

// Function to convert a terrain type to a character
char terrainToChar(TerrainType terrain);

// Function to save the current grid as an ASCII map
void saveGridAsASCII(const char* filename);

#endif // ASCII_MAP_H