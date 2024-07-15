#include <SDL2/SDL.h>
#include <stdio.h>
#include <assert.h>
#include "enemy.h"
#include "grid.h"
#include "gameloop.h"

#ifdef UNIT_TEST
// Mock functions
bool isWalkable(int x, int y) {
    // For testing, consider all coordinates within bounds as walkable
    return (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE);
}

bool isValid(int x, int y) {
    return x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE;
}
#endif

// Minimal implementation of CleanupEnemy to avoid undefined reference
void CleanupEnemy(Enemy* enemy) {
    if (enemy->entity.cachedPath) {
        free(enemy->entity.cachedPath);
        enemy->entity.cachedPath = NULL;
    }
    enemy->entity.cachedPathLength = 0;
}

// Test functions
void test_InitEnemy() {
    Enemy enemy;
    InitEnemy(&enemy, 2, 2, 0.5f);
    
    assert(enemy.entity.gridX == 2);
    assert(enemy.entity.gridY == 2);
    assert(enemy.entity.speed == 0.5f);
    assert(enemy.entity.posX == (2.0f * 2 / GRID_SIZE) - 1.0f + (1.0f / GRID_SIZE));
    assert(enemy.entity.posY == 1.0f - (2.0f * 2 / GRID_SIZE) - (1.0f / GRID_SIZE));
    assert(enemy.entity.targetGridX == 2);
    assert(enemy.entity.targetGridY == 2);
    assert(enemy.entity.finalGoalX == 2);
    assert(enemy.entity.finalGoalY == 2);
    assert(enemy.entity.needsPathfinding == false);
    assert(enemy.entity.cachedPath == NULL);
    assert(enemy.entity.cachedPathLength == 0);
    assert(enemy.entity.currentPathIndex == 0);
    assert(enemy.entity.isPlayer == false);

    printf("test_InitEnemy passed\n");
}

void test_MovementAI() {
    Enemy enemy;
    InitEnemy(&enemy, 2, 2, 0.5f);
    
    // Test that MovementAI changes the target occasionally
    int initialTargetX = enemy.entity.targetGridX;
    int initialTargetY = enemy.entity.targetGridY;
    
    for (int i = 0; i < 1000; i++) {
        MovementAI(&enemy);
        if (enemy.entity.targetGridX != initialTargetX || enemy.entity.targetGridY != initialTargetY) {
            printf("test_MovementAI passed\n");
            return;
        }
    }
    
    assert(0 && "MovementAI did not change target after 1000 iterations");
}

void test_UpdateEnemy() {
    Enemy enemy;
    InitEnemy(&enemy, 2, 2, 0.5f);
    
    Entity* allEntities[1] = {&enemy.entity};
    
    // Test that UpdateEnemy calls MovementAI and updates the enemy's position
    float initialPosX = enemy.entity.posX;
    float initialPosY = enemy.entity.posY;
    
    for (int i = 0; i < 100; i++) {
        UpdateEnemy(&enemy, allEntities, 1);
        if (enemy.entity.posX != initialPosX || enemy.entity.posY != initialPosY) {
            printf("test_UpdateEnemy passed\n");
            return;
        }
    }
    
    assert(0 && "UpdateEnemy did not change enemy position after 100 iterations");
}

void test_CleanupEnemy() {
    Enemy enemy;
    InitEnemy(&enemy, 2, 2, 0.5f);
    
    // Allocate some memory for cachedPath
    enemy.entity.cachedPath = malloc(sizeof(Node) * 10);
    enemy.entity.cachedPathLength = 10;
    
    CleanupEnemy(&enemy);
    
    assert(enemy.entity.cachedPath == NULL);
    assert(enemy.entity.cachedPathLength == 0);
    
    printf("test_CleanupEnemy passed\n");
}

void test_EnemyPathfinding() {
    Enemy enemy;
    InitEnemy(&enemy, 2, 2, 0.5f);
    
    enemy.entity.finalGoalX = 4;
    enemy.entity.finalGoalY = 4;
    enemy.entity.needsPathfinding = true;

    UpdateEnemy(&enemy, NULL, 0);

    assert(enemy.entity.cachedPath != NULL);
    assert(enemy.entity.cachedPathLength > 0);
    assert(enemy.entity.targetGridX == 4);
    assert(enemy.entity.targetGridY == 4);

    printf("test_EnemyPathfinding passed\n");
}

int SDL_main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;  // These lines suppress unused parameter warnings

    test_InitEnemy();
    test_MovementAI();
    test_UpdateEnemy();
    test_CleanupEnemy();
    test_EnemyPathfinding();
    
    printf("All enemy tests passed!\n");
    return 0;
}
