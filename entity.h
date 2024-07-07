// entity.h
#ifndef ENTITY_H
#define ENTITY_H

typedef struct {
    float posX;
    float posY;
    float targetPosX;
    float targetPosY;
    float moveSpeed;
} Entity;

void InitEntity(Entity* entity, float startX, float startY, float speed);
void UpdateEntity(Entity* entity);

#endif // ENTITY_H
