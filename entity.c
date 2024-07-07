// entity.c
#include "entity.h"
#include <math.h>

void InitEntity(Entity* entity, float startX, float startY, float speed) {
    entity->posX = startX;
    entity->posY = startY;
    entity->targetPosX = startX;
    entity->targetPosY = startY;
    entity->moveSpeed = speed;
}

void UpdateEntity(Entity* entity) {
    float dx = entity->targetPosX - entity->posX;
    float dy = entity->targetPosY - entity->posY;
    float distance = sqrt(dx * dx + dy * dy);

    if (distance > entity->moveSpeed) {
        float normalizedDx = dx / distance;
        float normalizedDy = dy / distance;
        entity->posX += normalizedDx * entity->moveSpeed;
        entity->posY += normalizedDy * entity->moveSpeed;
    } else {
        entity->posX = entity->targetPosX;
        entity->posY = entity->targetPosY;
    }
}
