#include "structures.h"
#include <stdio.h>
#include <math.h>
#include "grid.h"
#include "gameloop.h"
#include "player.h"
#include <stdint.h>
#include <inttypes.h>
#include "ui.h"
// Constants for texture coordinates from your existing system

/* These defs need to be in structures.c since they incl the division operation - do not move to header file or they will break when refed*/
#define WALL_FRONT_TEX_X (1.0f / 3.0f)
#define WALL_FRONT_TEX_Y (3.0f / 6.0f)
#define WALL_VERTICAL_TEX_X (0.0f / 3.0f)
#define WALL_VERTICAL_TEX_Y (3.0f / 6.0f)
#define WALL_TOP_LEFT_TEX_X (1.0f / 3.0f)
#define WALL_TOP_LEFT_TEX_Y (2.0f / 6.0f)
#define WALL_TOP_RIGHT_TEX_X (2.0f / 3.0f)
#define WALL_TOP_RIGHT_TEX_Y (3.0f / 6.0f)
#define WALL_BOTTOM_LEFT_TEX_X (0.0f / 3.0f)
#define WALL_BOTTOM_LEFT_TEX_Y (2.0f / 6.0f)
#define WALL_BOTTOM_RIGHT_TEX_X (2.0f / 3.0f)
#define WALL_BOTTOM_RIGHT_TEX_Y (2.0f / 6.0f)

#define DOOR_VERTICAL_TEX_X (2.0f / 3.0f)    // Middle column
#define DOOR_VERTICAL_TEX_Y (1.0f / 6.0f)    // Top row
#define DOOR_HORIZONTAL_TEX_X (0.0f / 3.0f)  // First column 
#define DOOR_HORIZONTAL_TEX_Y (1.0f / 6.0f)  // Top row
#define DOOR_VERTICAL_OPEN_TEX_X (0.0f / 3.0f)    // Middle column
#define DOOR_VERTICAL_OPEN_TEX_Y (0.0f / 6.0f)    // Top row
#define DOOR_HORIZONTAL_OPEN_TEX_X (1.0f / 3.0f)  // First column 
#define DOOR_HORIZONTAL_OPEN_TEX_Y (1.0f / 6.0f)  // Top row
EnclosureManager globalEnclosureManager;

void initializeStructureSystem(void) {
    printf("Structure system initialized\n");
}

const char* getStructureName(StructureType type) {
    switch(type) {
        case STRUCTURE_WALL: return "Wall";
        case STRUCTURE_DOOR: return "Door";
        default: return "Unknown";
    }
}

bool canPlaceStructure(StructureType type, int gridX, int gridY) {
    // Basic bounds checking
    if (gridX < 0 || gridX >= GRID_SIZE || gridY < 0 || gridY >= GRID_SIZE) {
        printf("Out of bounds\n");
        return false;
    }

    // Check if tile is already occupied
    if (grid[gridY][gridX].structureType != 0) {
        printf("Tile already occupied\n");
        return false;
    }

    switch (type) {
        case STRUCTURE_WALL:
            // Walls can be placed on any empty tile
            return true;

        case STRUCTURE_DOOR: {
            printf("Checking door at (%d,%d)\n", gridX, gridY);
            
            // Check specifically for WALLS (not just any structure)
            bool hasNorth = (gridY > 0) && grid[gridY-1][gridX].structureType == STRUCTURE_WALL;
            bool hasSouth = (gridY < GRID_SIZE-1) && grid[gridY+1][gridX].structureType == STRUCTURE_WALL;
            bool hasEast = (gridX < GRID_SIZE-1) && grid[gridY][gridX+1].structureType == STRUCTURE_WALL;
            bool hasWest = (gridX > 0) && grid[gridY][gridX-1].structureType == STRUCTURE_WALL;
            
            printf("Walls: N:%d S:%d E:%d W:%d\n", hasNorth, hasSouth, hasEast, hasWest);
            
            bool valid = hasNorth || hasSouth || hasEast || hasWest;
            printf("Door placement: %s\n", valid ? "valid" : "invalid");
            return valid;
        }

        default:
            return false;
    }
}
void updateWallTextures(int gridX, int gridY) {
    if (grid[gridY][gridX].structureType == 0) return;

    // Preserve door textures
    if ((grid[gridY][gridX].wallTexX == DOOR_VERTICAL_TEX_X && 
         grid[gridY][gridX].wallTexY == DOOR_VERTICAL_TEX_Y) ||
        (grid[gridY][gridX].wallTexX == DOOR_HORIZONTAL_TEX_X &&
         grid[gridY][gridX].wallTexY == DOOR_HORIZONTAL_TEX_Y)) {
        return;
    }

    // Get adjacent wall info
    bool hasNorth = (gridY > 0) && grid[gridY-1][gridX].structureType != 0;
    bool hasSouth = (gridY < GRID_SIZE-1) && grid[gridY+1][gridX].structureType != 0;
    bool hasEast = (gridX < GRID_SIZE-1) && grid[gridY][gridX+1].structureType != 0;
    bool hasWest = (gridX > 0) && grid[gridY][gridX-1].structureType != 0;

    // Handle corner walls
    if (hasNorth && hasEast && !hasWest && !hasSouth) {
        grid[gridY][gridX].wallTexX = WALL_BOTTOM_LEFT_TEX_X;
        grid[gridY][gridX].wallTexY = WALL_BOTTOM_LEFT_TEX_Y;
    } 
    else if (hasNorth && hasWest && !hasEast && !hasSouth) {
        grid[gridY][gridX].wallTexX = WALL_BOTTOM_RIGHT_TEX_X;
        grid[gridY][gridX].wallTexY = WALL_BOTTOM_RIGHT_TEX_Y;
    } 
    else if (hasSouth && hasEast && !hasWest && !hasNorth) {
        grid[gridY][gridX].wallTexX = WALL_TOP_LEFT_TEX_X;
        grid[gridY][gridX].wallTexY = WALL_TOP_LEFT_TEX_Y;
    } 
    else if (hasSouth && hasWest && !hasEast && !hasNorth) {
        grid[gridY][gridX].wallTexX = WALL_TOP_RIGHT_TEX_X;
        grid[gridY][gridX].wallTexY = WALL_TOP_RIGHT_TEX_Y;
    } 
    // Handle vertical walls
    else if (hasNorth || hasSouth) {
        grid[gridY][gridX].wallTexX = WALL_VERTICAL_TEX_X;
        grid[gridY][gridX].wallTexY = WALL_VERTICAL_TEX_Y;
    } 
    // Handle front-facing walls
    else {
        grid[gridY][gridX].wallTexX = WALL_FRONT_TEX_X;
        grid[gridY][gridX].wallTexY = WALL_FRONT_TEX_Y;
    }
}
void updateSurroundingStructures(int gridX, int gridY) {
    // Update the placed structure and all adjacent cells
    if (gridY > 0) updateWallTextures(gridX, gridY-1);
    if (gridY < GRID_SIZE-1) updateWallTextures(gridX, gridY+1);
    if (gridX > 0) updateWallTextures(gridX-1, gridY);
    if (gridX < GRID_SIZE-1) updateWallTextures(gridX+1, gridY);
    updateWallTextures(gridX, gridY);
}


bool placeStructure(StructureType type, int gridX, int gridY) {
   if (!canPlaceStructure(type, gridX, gridY)) {
       return false;
   }

   // Check if an entity is targeting the tile
   if (isEntityTargetingTile(gridX, gridY)) {
       printf("Cannot place structure at (%d, %d): entity is targeting this tile.\n", gridX, gridY);
       return false;
   }

   // Set the structure type and make it unwalkable
   grid[gridY][gridX].structureType = type;
   grid[gridY][gridX].materialType = MATERIAL_WOOD; // Default to wood for now
   GRIDCELL_SET_WALKABLE(grid[gridY][gridX], false);

   switch(type) {
       case STRUCTURE_WALL:
           updateSurroundingStructures(gridX, gridY);
           break;
           
       case STRUCTURE_DOOR: {
           // Determine door orientation based on adjacent walls
           bool hasNorth = (gridY > 0) && grid[gridY-1][gridX].structureType != 0;
           bool hasSouth = (gridY < GRID_SIZE-1) && grid[gridY+1][gridX].structureType != 0;

           // Set door orientation based on adjacent walls
           if (hasNorth || hasSouth) {
               // Vertical door
               GRIDCELL_SET_ORIENTATION(grid[gridY][gridX], 0);
               grid[gridY][gridX].wallTexX = DOOR_VERTICAL_TEX_X;
               grid[gridY][gridX].wallTexY = DOOR_VERTICAL_TEX_Y;
           } else {
               // Horizontal door
               GRIDCELL_SET_ORIENTATION(grid[gridY][gridX], 1);
               grid[gridY][gridX].wallTexX = DOOR_HORIZONTAL_TEX_X;
               grid[gridY][gridX].wallTexY = DOOR_HORIZONTAL_TEX_Y;
           }
           
           updateSurroundingStructures(gridX, gridY);
           break;
       }

       case STRUCTURE_PLANT:
           // Plants start as unwalkable but could become walkable when harvested
           GRIDCELL_SET_WALKABLE(grid[gridY][gridX], false);
           grid[gridY][gridX].materialType = FERN; // Default plant type for now
           break;

       default:
           return false;
   }

   // After successful placement, check for enclosures
   Enclosure enclosure = detectEnclosure(gridX, gridY);
   if (enclosure.isValid) {
       EnclosureData newEnclosure = {0};
       Point* boundaryPoints = malloc(enclosure.tileCount * sizeof(Point));
       int wallCount = 0;
       int doorCount = 0;
       
       int sumX = 0, sumY = 0;
       for (int i = 0; i < enclosure.tileCount; i++) {
           int tileX = enclosure.tiles[i] % GRID_SIZE;
           int tileY = enclosure.tiles[i] / GRID_SIZE;
           
           boundaryPoints[i].x = tileX;
           boundaryPoints[i].y = tileY;
           
           sumX += tileX;
           sumY += tileY;
           
           if (grid[tileY][tileX].structureType != 0) {
               if (grid[tileY][tileX].structureType == STRUCTURE_DOOR) {
                   doorCount++;
               } else {
                   wallCount++;
               }
           }
       }

       newEnclosure.hash = calculateEnclosureHash(boundaryPoints, enclosure.tileCount, enclosure.tileCount);
       newEnclosure.boundaryTiles = boundaryPoints;
       newEnclosure.boundaryCount = enclosure.tileCount;
       newEnclosure.totalArea = enclosure.tileCount;
       newEnclosure.centerPoint.x = sumX / enclosure.tileCount;
       newEnclosure.centerPoint.y = sumY / enclosure.tileCount;
       newEnclosure.doorCount = doorCount;
       newEnclosure.wallCount = wallCount;
       newEnclosure.isValid = true;

       bool isNewEnclosure = true;
       for (int i = 0; i < globalEnclosureManager.count; i++) {
           if (globalEnclosureManager.enclosures[i].hash == newEnclosure.hash) {
               isNewEnclosure = false;
               break;
           }
       }

       if (isNewEnclosure) {
           addEnclosure(&globalEnclosureManager, &newEnclosure);
           extern Player player;
           awardConstructionExp(&player, &newEnclosure);
           
           printf("Found new enclosure with %d tiles! Hash: %" PRIu64 "\n", 
               enclosure.tileCount, newEnclosure.hash);
           printf("Center point: (%d, %d)\n", 
                  newEnclosure.centerPoint.x, newEnclosure.centerPoint.y);
           printf("Walls: %d, Doors: %d\n", wallCount, doorCount);
       }

       free(enclosure.tiles);
   }

   printf("Placed %s at grid position: %d, %d\n", getStructureName(type), gridX, gridY);
   return true;
}
void cleanupStructureSystem(void) {
    printf("Structure system cleaned up\n");
}

void cycleStructureType(PlacementMode* mode, bool forward) {
    if (!mode) return;

    if (forward) {
        mode->currentType = (mode->currentType + 1);
        if (mode->currentType >= STRUCTURE_PLANT) {  // Skip plant and wrap back to wall
            mode->currentType = STRUCTURE_WALL;
        }
    } else {
        mode->currentType = (mode->currentType - 1);
        if (mode->currentType <= STRUCTURE_NONE) {  // If we hit NONE or below, wrap to door
            mode->currentType = STRUCTURE_DOOR;
        }
    }

    printf("Selected structure: %s\n", getStructureName(mode->currentType));
}

bool isEntityTargetingTile(int gridX, int gridY) {
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (allEntities[i]) {
            // Check if entity is currently on the tile
            if (allEntities[i]->gridX == gridX && allEntities[i]->gridY == gridY) {
                return true;
            }
            // Check if entity's next movement target is the tile
            if (allEntities[i]->targetGridX == gridX && allEntities[i]->targetGridY == gridY) {
                return true;
            }
        }
    }
    return false;
}

AdjacentTile findNearestAdjacentTile(int targetX, int targetY, int fromX, int fromY, bool requireWalkable) {
    AdjacentTile result = {-1, -1, INFINITY};
    
    // Check all adjacent tiles to the target position
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            
            int checkX = targetX + dx;
            int checkY = targetY + dy;
            
            // Ensure tile is in bounds and meets walkability requirement
            if (checkX >= 0 && checkX < GRID_SIZE && 
                checkY >= 0 && checkY < GRID_SIZE && 
                (!requireWalkable || GRIDCELL_IS_WALKABLE(grid[checkY][checkX]))) {
                
                float dist = sqrtf(powf(fromX - checkX, 2) + 
                                 powf(fromY - checkY, 2));
                
                if (dist < result.distance) {
                    result.x = checkX;
                    result.y = checkY;
                    result.distance = dist;
                }
            }
        }
    }
    
    return result;
}

bool toggleDoor(int gridX, int gridY, Player* player) {
    // Verify it's a door
    if (grid[gridY][gridX].structureType != STRUCTURE_DOOR) return false;

    bool isNearby = (
        abs(gridX - player->entity.gridX) <= 1 && 
        abs(gridY - player->entity.gridY) <= 1
    );

    if (isNearby) {
        // Toggle door walkability
        bool currentlyOpen = GRIDCELL_IS_WALKABLE(grid[gridY][gridX]);
        GRIDCELL_SET_WALKABLE(grid[gridY][gridX], !currentlyOpen);
        
        // Get orientation and current state
        bool isVertical = (GRIDCELL_GET_ORIENTATION(grid[gridY][gridX]) == 0);
        bool willBeOpen = !currentlyOpen;  // What state it will be after toggling

        // Update textures based on orientation and state
        if (isVertical) {
            if (willBeOpen) {
                grid[gridY][gridX].wallTexX = DOOR_VERTICAL_OPEN_TEX_X;
                grid[gridY][gridX].wallTexY = DOOR_VERTICAL_OPEN_TEX_Y;
            } else {
                grid[gridY][gridX].wallTexX = DOOR_VERTICAL_TEX_X;
                grid[gridY][gridX].wallTexY = DOOR_VERTICAL_TEX_Y;
            }
        } else {
            if (willBeOpen) {
                grid[gridY][gridX].wallTexX = DOOR_HORIZONTAL_OPEN_TEX_X;
                grid[gridY][gridX].wallTexY = DOOR_HORIZONTAL_OPEN_TEX_Y;
            } else {
                grid[gridY][gridX].wallTexX = DOOR_HORIZONTAL_TEX_X;
                grid[gridY][gridX].wallTexY = DOOR_HORIZONTAL_TEX_Y;
            }
        }
        
        return true;
    } else {
        // Path to nearest door-adjacent tile if not nearby
        AdjacentTile nearest = findNearestAdjacentTile(gridX, gridY,
                                                      player->entity.gridX, 
                                                      player->entity.gridY,
                                                      true);
        if (nearest.x != -1) {
            player->entity.finalGoalX = nearest.x;
            player->entity.finalGoalY = nearest.y;
            player->entity.targetGridX = player->entity.gridX;
            player->entity.targetGridY = player->entity.gridY;
            player->entity.needsPathfinding = true;
            return true;
        }
    }
    return false;
}
bool isWallOrDoor(int x, int y) {
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) return false;
return grid[y][x].structureType != 0;

}

Enclosure detectEnclosure(int startX, int startY) {
    Enclosure result = {NULL, 0, false, 0};
    
    // Validate input coordinates first
    if (startX < 0 || startX >= GRID_SIZE || startY < 0 || startY >= GRID_SIZE) {
        return result;
    }
    
    // Check if starting position is valid
    if (!isWallOrDoor(startX, startY)) {
        return result;
    }
    
    // Allocate memory with null checks
    bool* visited = calloc(GRID_SIZE * GRID_SIZE, sizeof(bool));
    if (!visited) {
        printf("Failed to allocate visited array\n");
        return result;
    }
    
    int* wallPath = malloc(GRID_SIZE * GRID_SIZE * sizeof(int));
    if (!wallPath) {
        printf("Failed to allocate wall path array\n");
        free(visited);
        return result;
    }
    
    PathNode* stack = malloc(GRID_SIZE * GRID_SIZE * sizeof(PathNode));
    if (!stack) {
        printf("Failed to allocate stack\n");
        free(visited);
        free(wallPath);
        return result;
    }

    int pathLength = 0;
    int stackSize = 0;
    
    // Initialize first node
    PathNode firstNode;
    firstNode.x = startX;
    firstNode.y = startY;
    firstNode.prevX = -1;
    firstNode.prevY = -1;
    firstNode.direction = -1;
    stack[stackSize++] = firstNode;
    
    bool foundCycle = false;
    
    while (stackSize > 0 && !foundCycle) {
        PathNode current = stack[--stackSize];
        
        if (visited[current.y * GRID_SIZE + current.x]) continue;
        
        visited[current.y * GRID_SIZE + current.x] = true;
        wallPath[pathLength++] = current.y * GRID_SIZE + current.x;
        
        int dx[] = {0, 1, 0, -1};
        int dy[] = {-1, 0, 1, 0};
        
        for (int i = 0; i < 4; i++) {
            int newX = current.x + dx[i];
            int newY = current.y + dy[i];
            
            if (newX == current.prevX && newY == current.prevY) continue;
            
            if (isWallOrDoor(newX, newY)) {
                if (newX == startX && newY == startY && pathLength > 2) {
                    foundCycle = true;
                    break;
                }
                PathNode nextNode;
                nextNode.x = newX;
                nextNode.y = newY;
                nextNode.prevX = current.x;
                nextNode.prevY = current.y;
                nextNode.direction = i;
                stack[stackSize++] = nextNode;
            }
        }
    }

    if (!foundCycle) {
        free(wallPath);
        free(visited);
        free(stack);
        return result;
    }

    // Only if we found a cycle do we perform the expensive calculations
    if (foundCycle) {
        Point* boundaryPoints = malloc(pathLength * sizeof(Point));
        for (int i = 0; i < pathLength; i++) {
            boundaryPoints[i].x = wallPath[i] % GRID_SIZE;
            boundaryPoints[i].y = wallPath[i] / GRID_SIZE;
        }

        int area = pathLength;  // For now just using boundary length
        result.hash = calculateEnclosureHash(boundaryPoints, pathLength, area);
        result.tiles = wallPath;
        result.tileCount = pathLength;
        result.isValid = true;

        free(boundaryPoints);
    } else {
        free(wallPath);
    }
    
    free(visited);
    free(stack);
    return result;
}

#define FNV_PRIME 1099511628211ULL
#define FNV_OFFSET 14695981039346656037ULL

uint64_t calculateEnclosureHash(Point* boundaryTiles, int boundaryCount, int totalArea) {
    uint64_t hash = FNV_OFFSET;

    // Sort boundary tiles for consistent hashing
    // Using a simple bubble sort for now
    for (int i = 0; i < boundaryCount - 1; i++) {
        for (int j = 0; j < boundaryCount - i - 1; j++) {
            if (boundaryTiles[j].y > boundaryTiles[j + 1].y || 
                (boundaryTiles[j].y == boundaryTiles[j + 1].y && 
                 boundaryTiles[j].x > boundaryTiles[j + 1].x)) {
                Point temp = boundaryTiles[j];
                boundaryTiles[j] = boundaryTiles[j + 1];
                boundaryTiles[j + 1] = temp;
            }
        }
    }

    // Hash boundary tiles
    for (int i = 0; i < boundaryCount; i++) {
        hash *= FNV_PRIME;
        hash ^= boundaryTiles[i].x;
        hash *= FNV_PRIME;
        hash ^= boundaryTiles[i].y;
    }

    // Incorporate area into hash
    hash *= FNV_PRIME;
    hash ^= totalArea;

    return hash;
}

void initEnclosureManager(EnclosureManager* manager) {
    manager->capacity = 16;  // Initial capacity
    manager->count = 0;
    manager->enclosures = malloc(manager->capacity * sizeof(EnclosureData));
    if (!manager->enclosures) {
        fprintf(stderr, "Failed to initialize enclosure manager\n");
        exit(1);
    }
}

void addEnclosure(EnclosureManager* manager, EnclosureData* enclosure) {
    // Check if we need to resize
    if (manager->count >= manager->capacity) {
        manager->capacity *= 2;
        EnclosureData* newArray = realloc(manager->enclosures, 
                                         manager->capacity * sizeof(EnclosureData));
        if (!newArray) {
            fprintf(stderr, "Failed to resize enclosure manager\n");
            return;
        }
        manager->enclosures = newArray;
    }

    // Check if enclosure already exists
    for (int i = 0; i < manager->count; i++) {
        if (manager->enclosures[i].hash == enclosure->hash) {
            printf("Enclosure already exists with hash %" PRIu64 "\n", enclosure->hash);
            return;
        }
    }

    // Add new enclosure
    manager->enclosures[manager->count] = *enclosure;
    manager->count++;
    printf("Added new enclosure with hash %" PRIu64 " (total: %d)\n",
       enclosure->hash, manager->count);
}

EnclosureData* findEnclosure(EnclosureManager* manager, uint64_t hash) {
    for (int i = 0; i < manager->count; i++) {
        if (manager->enclosures[i].hash == hash) {
            return &manager->enclosures[i];
        }
    }
    return NULL;
}

void removeEnclosure(EnclosureManager* manager, uint64_t hash) {
    for (int i = 0; i < manager->count; i++) {
        if (manager->enclosures[i].hash == hash) {
            // Free enclosure data
            free(manager->enclosures[i].boundaryTiles);
            free(manager->enclosures[i].interiorTiles);

            // Move last enclosure to this position if it's not the last one
            if (i < manager->count - 1) {
                manager->enclosures[i] = manager->enclosures[manager->count - 1];
            }
            
            manager->count--;
            printf("Removed enclosure with hash %" PRIu64 " (remaining: %d)\n",
       hash, manager->count);
            return;
        }
    }
}

void cleanupEnclosureManager(EnclosureManager* manager) {
    for (int i = 0; i < manager->count; i++) {
        free(manager->enclosures[i].boundaryTiles);
        free(manager->enclosures[i].interiorTiles);
    }
    free(manager->enclosures);
    manager->enclosures = NULL;
    manager->count = 0;
    manager->capacity = 0;
}

void awardConstructionExp(Player* player, const EnclosureData* enclosure) {
    // Base exp values
    const float BASE_WALL_EXP = 10.0f;
    const float BASE_DOOR_EXP = 25.0f;
    const float AREA_MULTIPLIER = 5.0f;

    // Calculate total exp
    float wallExp = BASE_WALL_EXP * enclosure->wallCount;
    float doorExp = BASE_DOOR_EXP * enclosure->doorCount;
    float areaExp = AREA_MULTIPLIER * enclosure->totalArea;
    
    float totalExp = wallExp + doorExp + areaExp;

    // Store old exp and level for progress calculation
    float oldExp = player->skills.experience[SKILL_CONSTRUCTION];
    int oldLevel = player->skills.levels[SKILL_CONSTRUCTION];
    
    // Add new exp
    player->skills.experience[SKILL_CONSTRUCTION] += totalExp;
    int newLevel = (int)(player->skills.experience[SKILL_CONSTRUCTION] / EXP_PER_LEVEL);
    player->skills.levels[SKILL_CONSTRUCTION] = newLevel;

    // Calculate progress percentages
    float oldProgress = fmodf(oldExp, EXP_PER_LEVEL) / EXP_PER_LEVEL * 100.0f;
    float newProgress = fmodf(player->skills.experience[SKILL_CONSTRUCTION], EXP_PER_LEVEL) / EXP_PER_LEVEL * 100.0f;

    printf("\n=== Construction Experience Award ===\n");
    printf("Base Calculations:\n");
    printf("- Wall exp (%.0f per wall): %.0f (walls: %d)\n", BASE_WALL_EXP, wallExp, enclosure->wallCount);
    printf("- Door exp (%.0f per door): %.0f (doors: %d)\n", BASE_DOOR_EXP, doorExp, enclosure->doorCount);
    printf("- Area exp (%.0f per tile): %.0f (area: %d)\n", AREA_MULTIPLIER, areaExp, enclosure->totalArea);
    printf("Total exp awarded: %.0f\n\n", totalExp);

    printf("Progress Update:\n");
    printf("- Total exp: %.1f -> %.1f\n", oldExp, player->skills.experience[SKILL_CONSTRUCTION]);
    printf("- Level: %d -> %d\n", oldLevel, newLevel);
    printf("- Progress to next level: %.1f%% -> %.1f%%\n", oldProgress, newProgress);
    
    if (newLevel > oldLevel) {
        printf("\n*** LEVEL UP! ***\n");
        printf("Construction level increased from %d to %d!\n", oldLevel, newLevel);
    }
    printf("==============================\n\n");
}