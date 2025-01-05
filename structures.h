#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stdint.h>
#include <stdbool.h>
#include "grid.h"
#include "player.h"
#include "structure_types.h"

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
} Point;

typedef struct {
    uint64_t hash;               // Combined hash of boundary and area
    Point* boundaryTiles;        // Array of wall/door positions
    int boundaryCount;           // Number of boundary tiles
    Point* interiorTiles;        // Array of interior positions
    int interiorCount;           // Number of interior tiles
    int totalArea;              // Total enclosed area
    Point centerPoint;          // Center point of the enclosure
    int doorCount;              // Number of doors
    int wallCount;              // Number of walls
    bool isValid;               // Whether the enclosure is still valid
} EnclosureData;

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
bool isWallOrDoor(int x, int y);
Enclosure detectEnclosure(int startX, int startY);
#endif // STRUCTURES_H