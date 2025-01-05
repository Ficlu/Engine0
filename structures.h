#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stdbool.h>
#include "grid.h"


typedef enum {
    STRUCTURE_NONE = 0,
    STRUCTURE_WALL,
    STRUCTURE_DOOR,
    STRUCTURE_COUNT
} StructureType;

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

#endif // STRUCTURES_H