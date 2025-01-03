#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"

typedef struct {
    Entity entity;
    float cameraTargetX;
    float cameraTargetY;
    float cameraCurrentX;
    float cameraCurrentY;
    float cameraSpeed;
    float lookAheadX;
    float lookAheadY;
    float zoomFactor;
    int targetBuildX;      // New: Coordinates where we want to build
    int targetBuildY;      // New: Coordinates where we want to build
    bool hasBuildTarget;   // New: Flag to indicate if we have a pending build
} Player;

void InitPlayer(Player* player, int startGridX, int startGridY, float speed);
void UpdatePlayer(Player* player, Entity** allEntities, int entityCount);
void CleanupPlayer(Player* player);

#endif // PLAYER_H
