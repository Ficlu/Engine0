#include "grid.h"
#include <stdlib.h>

int GRID_SIZE;
Tile** grid;

void initializeGrid(int size) {
    GRID_SIZE = size;
    grid = (Tile**)malloc(sizeof(Tile*) * GRID_SIZE);
    for (int i = 0; i < GRID_SIZE; i++) {
        grid[i] = (Tile*)malloc(sizeof(Tile) * GRID_SIZE);
    }
    
    // Initialize grid tiles
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            grid[y][x].walkable = (rand() % 10 < 8);  // 80% chance of being walkable
        }
    }

    // Ensure there's at least one walkable path across the grid
    for (int x = 0; x < GRID_SIZE; x++) {
        grid[GRID_SIZE / 2][x].walkable = true;
    }
}

void cleanupGrid() {
    for (int i = 0; i < GRID_SIZE; i++) {
        free(grid[i]);
    }
    free(grid);
}

bool isValid(int x, int y) {
    return x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE;
}

bool isFullyWalkable(int x, int y) {
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