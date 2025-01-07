#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"
#include "structure_types.h"
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
    StructureType pendingBuildType; 
        struct {
        uint32_t construction;    // Using uint32_t for efficient memory alignment
        float constructionExp;    // Fractional part for smooth bar movement
    } skills;
} Player;

void InitPlayer(Player* player, int startGridX, int startGridY, float speed);
void UpdatePlayer(Player* player, Entity** allEntities, int entityCount);
void CleanupPlayer(Player* player);

#endif // PLAYER_H
