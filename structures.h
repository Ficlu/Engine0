// structures.h
#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stdint.h>
#include <stdbool.h>
#include "grid.h"
#include "structure_types.h"

#include "player.h"

#include "enclosure_types.h"

typedef struct {
    int* tiles;  // Array of tile indices within the enclosure
    int tileCount;
    bool isValid;
    uint64_t hash; 
} Enclosure;

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

typedef struct {
    int x, y;
    int prevX, prevY;
    int direction;
} PathNode;

typedef struct {
    EnclosureData* enclosures;  // Dynamic array of enclosures
    int count;                  // Number of active enclosures
    int capacity;              // Current capacity of the array
} EnclosureManager;



// Function declarations
uint64_t calculateEnclosureHash(Point* boundaryTiles, int boundaryCount, int totalArea);
void initEnclosureManager(EnclosureManager* manager);
void addEnclosure(EnclosureManager* manager, EnclosureData* enclosure);
EnclosureData* findEnclosure(EnclosureManager* manager, uint64_t hash);
void removeEnclosure(EnclosureManager* manager, uint64_t hash);
void cleanupEnclosureManager(EnclosureManager* manager);
extern EnclosureManager globalEnclosureManager; 
bool isWithinBuildRange(float entityX, float entityY, int targetGridX, int targetGridY);
struct Player;  // Forward declaration
void cycleStructureType(PlacementMode* mode, bool forward);
void initializeStructureSystem(void);
bool canPlaceStructure(StructureType type, int gridX, int gridY);
bool placeStructure(StructureType type, int gridX, int gridY, struct Player* player);
void updateSurroundingStructures(int gridX, int gridY);
const char* getStructureName(StructureType type);
void cleanupStructureSystem(void);
bool isEntityTargetingTile(int gridX, int gridY);
AdjacentTile findNearestAdjacentTile(int targetX, int targetY, int fromX, int fromY, bool requireWalkable);
bool toggleDoor(int gridX, int gridY, struct Player* player);
bool isWallOrDoor(int x, int y);
Enclosure detectEnclosure(int startX, int startY);

#endif // STRUCTURES_H