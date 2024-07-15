// grid.c
#include "grid.h"

Tile grid[GRID_SIZE][GRID_SIZE];

bool isValid(int x, int y) {
    return x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE;
}

bool isFullyWalkable(int x, int y) {
    // Check the tile itself and its immediate neighbors
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int checkX = x + dx;
            int checkY = y + dy;
            if (!isValid(checkX, checkY) || !isWalkable(checkX, checkY)) {
                return false;
            }
        }
    }
    return true;
}
bool isWalkable(int x, int y) {
    if (!isValid(x, y)) {
        return false;
    }
    return grid[y][x].walkable;
}