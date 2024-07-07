#ifndef ENEMY_H
#define ENEMY_H

#include "entity.h"

typedef struct {
    Entity entity;
} Enemy;

void InitEnemy(Enemy* enemy, float startX, float startY, float speed);
void UpdateEnemy(Enemy* enemy);
void MovementAI(Enemy* enemy);

#endif // ENEMY_H
