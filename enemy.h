// enemy.h
#ifndef ENEMY_H
#define ENEMY_H

#include "entity.h"

typedef struct {
    Entity entity;
    // Add any enemy-specific fields here
} Enemy;

void InitEnemy(Enemy* enemy, int startGridX, int startGridY, float speed);
void MovementAI(Enemy* enemy);
void UpdateEnemy(Enemy* enemy, Entity** allEntities, int entityCount);
void CleanupEnemy(Enemy* enemy);

#endif // ENEMY_H