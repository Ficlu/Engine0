#ifndef GRID_H
#define GRID_H

#include <stdbool.h>

#define GRID_SIZE 20  // Make sure this matches the definition in gameloop.h

typedef struct {
    bool walkable;
    // Add other tile properties here if needed
} Tile;

extern Tile grid[GRID_SIZE][GRID_SIZE];

bool isValid(int x, int y);
bool isFullyWalkable(int x, int y);
bool isWalkable(int x, int y);

#endif // GRID_H