#ifndef ENEMY_H
#define ENEMY_H

#include "entity.h"
#include <SDL2/SDL.h>

// Add the direction enum if not already defined elsewhere
typedef enum {
    ENEMY_DIR_DOWN,
    ENEMY_DIR_UP,
    ENEMY_DIR_LEFT,
    ENEMY_DIR_RIGHT
} EnemyDirection;

// Add animation structure
typedef struct {
    uint8_t currentFrame;    
    Uint32 lastFrameUpdate;  
    bool isMoving;     
    EnemyDirection facing;     
} EnemyAnimation;

typedef struct {
    Entity entity;
    Uint32 lastPathfindingTime;
    EnemyAnimation* animation;  // Add this line
} Enemy;

void InitEnemy(Enemy* enemy, int startGridX, int startGridY, float speed);
void MovementAI(Enemy* enemy, Uint32 currentTime);
void UpdateEnemy(Enemy* enemy, Entity** allEntities, int entityCount, Uint32 currentTime);
void CleanupEnemy(Enemy* enemy);

#endif // ENEMY_H