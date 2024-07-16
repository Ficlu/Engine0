#ifndef GRID_H
#define GRID_H

#include <stdbool.h>

extern int GRID_SIZE;

typedef struct {
    bool walkable;
    // Add other tile properties here if needed
} Tile;

extern Tile** grid;

void initializeGrid(int size);
void cleanupGrid();
bool isValid(int x, int y);
bool isFullyWalkable(int x, int y);
bool isWalkable(int x, int y);

#endif // GRID_H