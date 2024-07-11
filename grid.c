// grid.c
#include "grid.h"

Tile grid[GRID_SIZE][GRID_SIZE];

bool isValid(int x, int y) {
    return x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE;
}

bool isWalkable(int x, int y) {
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) {
        return false;
    }
    return grid[y][x].walkable;
}