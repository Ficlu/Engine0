// test_enemy.c
#include <stdio.h>
#include <stdlib.h>
#include "enemy.h"
#include "player.h"
#include "grid.h"
#include "pathfinding.h"
#include "entity.h"

// Setup function for initializing the grid and entities
void setup_grid(bool with_unwalkable) {
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            grid[y][x].walkable = !with_unwalkable || (rand() % 10 > 2);  // 20% unwalkable if with_unwalkable is true
        }
    }
}

void test_init_enemy() {
    Enemy enemy;
    InitEnemy(&enemy, 5, 5, 0.5f);
    printf("Test InitEnemy: ");
    if (enemy.entity.gridX == 5 && enemy.entity.gridY == 5 && enemy.entity.speed == 0.5f) {
        printf("Passed\n");
    } else {
        printf("Failed\n");
    }
}

void test_movement_ai() {
    Enemy enemy;
    InitEnemy(&enemy, 5, 5, 0.5f);
    int initialX = enemy.entity.targetGridX;
    int initialY = enemy.entity.targetGridY;
    MovementAI(&enemy);
    printf("Test MovementAI: ");
    if (enemy.entity.targetGridX != initialX || enemy.entity.targetGridY != initialY) {
        printf("Passed\n");
    } else {
        printf("Failed\n");
    }
    printf("Initial Target: (%d, %d), New Target: (%d, %d)\n", initialX, initialY, enemy.entity.targetGridX, enemy.entity.targetGridY);
}

void test_update_enemy() {
    Enemy enemy;
    InitEnemy(&enemy, 5, 5, 0.5f);
    Entity* allEntities[1] = {&enemy.entity};
    UpdateEnemy(&enemy, allEntities, 1);
    printf("Test UpdateEnemy: ");
    if (enemy.entity.gridX != 5 || enemy.entity.gridY != 5 || !enemy.entity.needsPathfinding) {
        printf("Passed\n");
    } else {
        printf("Failed\n");
    }
}

void test_update_entity_path() {
    Enemy enemy;
    InitEnemy(&enemy, 5, 5, 0.5f);
    enemy.entity.targetGridX = 10;
    enemy.entity.targetGridY = 10;
    updateEntityPath(&enemy.entity);
    printf("Test UpdateEntityPath: ");
    if (enemy.entity.currentPath != NULL && enemy.entity.pathLength > 0) {
        printf("Passed\n");
    } else {
        printf("Failed\n");
    }
}

void test_unwalkable_tiles() {
    setup_grid(true);
    Enemy enemy;
    InitEnemy(&enemy, 5, 5, 0.5f);
    enemy.entity.targetGridX = 15;
    enemy.entity.targetGridY = 15;
    updateEntityPath(&enemy.entity);
    printf("Test UnwalkableTiles: ");
    if (enemy.entity.currentPath != NULL && enemy.entity.pathLength > 0) {
        printf("Passed\n");
    } else {
        printf("Failed\n");
    }
}

void test_non_direct_path() {
    setup_grid(false);
    grid[5][6].walkable = false;
    grid[5][7].walkable = false;
    grid[6][5].walkable = false;
    grid[7][5].walkable = false;
    
    Enemy enemy;
    InitEnemy(&enemy, 5, 5, 0.5f);
    enemy.entity.targetGridX = 10;
    enemy.entity.targetGridY = 10;
    updateEntityPath(&enemy.entity);
    printf("Test NonDirectPath: ");
    if (enemy.entity.currentPath != NULL && enemy.entity.pathLength > 0) {
        printf("Passed\n");
    } else {
        printf("Failed\n");
    }
}

void test_starting_positions() {
    setup_grid(false);
    printf("Test Starting Positions:\n");
    for (int x = 0; x < GRID_SIZE; x += 4) {
        for (int y = 0; y < GRID_SIZE; y += 4) {
            Enemy enemy;
            InitEnemy(&enemy, x, y, 0.5f);
            printf("  Enemy initialized at (%d, %d): ", x, y);
            if (enemy.entity.gridX == x && enemy.entity.gridY == y) {
                printf("Passed\n");
            } else {
                printf("Failed\n");
            }
        }
    }
}

void test_edge_cases() {
    setup_grid(false);
    Enemy enemy;
    InitEnemy(&enemy, 0, 0, 0.5f);
    enemy.entity.targetGridX = GRID_SIZE - 1;
    enemy.entity.targetGridY = GRID_SIZE - 1;
    updateEntityPath(&enemy.entity);
    printf("Test EdgeCases: ");
    if (enemy.entity.currentPath != NULL && enemy.entity.pathLength > 0) {
        printf("Passed\n");
    } else {
        printf("Failed\n");
    }
}

void test_obstacle_course() {
    setup_grid(false);
    for (int i = 5; i < 15; i++) {
        grid[10][i].walkable = false;
    }
    Enemy enemy;
    InitEnemy(&enemy, 0, 0, 0.5f);
    enemy.entity.targetGridX = 19;
    enemy.entity.targetGridY = 19;
    updateEntityPath(&enemy.entity);
    printf("Test ObstacleCourse: ");
    if (enemy.entity.currentPath != NULL && enemy.entity.pathLength > 0) {
        printf("Passed\n");
    } else {
        printf("Failed\n");
    }
}

int main() {
    setup_grid(false);
    test_init_enemy();
    test_movement_ai();
    test_update_enemy();
    test_update_entity_path();
    test_unwalkable_tiles();
    test_non_direct_path();
    test_starting_positions();
    test_edge_cases();
    test_obstacle_course();
    return 0;
}
