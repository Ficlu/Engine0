#ifndef ENTITY_H
#define ENTITY_H

typedef struct {
    float posX, posY;
    float targetPosX, targetPosY;
    float moveSpeed;
    int needsPathfinding;
} Entity;

void InitEntity(Entity* entity, float startX, float startY, float speed);
void UpdateEntity(Entity* entity);

#endif // ENTITY_H