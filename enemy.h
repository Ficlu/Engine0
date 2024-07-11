// enemy.h
#ifndef ENEMY_H
#define ENEMY_H

#include "entity.h"

typedef struct {
    Entity entity;
} Enemy;

void InitEnemy(Enemy* enemy, int startGridX, int startGridY, float speed);
void MovementAI(Enemy* enemy);

#endif // ENEMY_H