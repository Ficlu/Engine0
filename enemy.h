// enemy.h
#ifndef ENEMY_H
#define ENEMY_H

#include "entity.h"
#include <SDL2/SDL.h> // Add this line to get Uint32 definition

typedef struct {
    Entity entity;
    Uint32 lastPathfindingTime;
} Enemy;

void InitEnemy(Enemy* enemy, int startGridX, int startGridY, float speed);
void MovementAI(Enemy* enemy, Uint32 currentTime); // Update this line
void UpdateEnemy(Enemy* enemy, Entity** allEntities, int entityCount, Uint32 currentTime); // Update this line
void CleanupEnemy(Enemy* enemy);

#endif // ENEMY_H