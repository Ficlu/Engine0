#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stdbool.h>
#include "grid.h"
#include "player.h"
#include "structure_types.h"



typedef struct {
    int x;
    int y;
    float distance;
} AdjacentTile;

typedef struct {
    bool active;
    StructureType currentType;
    int previewX;
    int previewY;
    float opacity;
    bool validPlacement;
} PlacementMode;

// Function declarations
void cycleStructureType(PlacementMode* mode, bool forward);
void initializeStructureSystem(void);
bool canPlaceStructure(StructureType type, int gridX, int gridY);
bool placeStructure(StructureType type, int gridX, int gridY);
void updateSurroundingStructures(int gridX, int gridY);
const char* getStructureName(StructureType type);
void cleanupStructureSystem(void);
bool isEntityTargetingTile(int gridX, int gridY);
AdjacentTile findNearestAdjacentTile(int targetX, int targetY, int fromX, int fromY, bool requireWalkable);
bool toggleDoor(int gridX, int gridY, Player* player);
#endif // STRUCTURES_H