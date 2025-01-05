#include "structures.h"
#include <stdio.h>
#include <math.h>
#include "grid.h"
#include "gameloop.h"
#include "player.h"
#include <stdint.h>
#include <inttypes.h>
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
    if (grid[gridY][gridX].hasWall) {
        printf("Tile already occupied\n");
        return false;
    }

    switch (type) {
        case STRUCTURE_WALL:
            // Walls can be placed on any empty tile
            return true;

        case STRUCTURE_DOOR: {
            // Debug print current position
            printf("Checking door at (%d,%d)\n", gridX, gridY);
            
            // Check for walls
            bool hasNorth = (gridY > 0) && grid[gridY-1][gridX].hasWall;
            bool hasSouth = (gridY < GRID_SIZE-1) && grid[gridY+1][gridX].hasWall;
            bool hasEast = (gridX < GRID_SIZE-1) && grid[gridY][gridX+1].hasWall;
            bool hasWest = (gridX > 0) && grid[gridY][gridX-1].hasWall;
            
            // Debug print wall configuration
            printf("Walls: N:%d S:%d E:%d W:%d\n", hasNorth, hasSouth, hasEast, hasWest);
            
            // Check for at least one wall
            bool valid = hasNorth || hasSouth || hasEast || hasWest;

            printf("Door placement: %s\n", valid ? "valid" : "invalid");
            return valid;
        }

        default:
            return false;
    }
}

void updateWallTextures(int gridX, int gridY) {
    if (!grid[gridY][gridX].hasWall) return;

    // Preserve door textures
    if ((grid[gridY][gridX].wallTexX == DOOR_VERTICAL_TEX_X && 
         grid[gridY][gridX].wallTexY == DOOR_VERTICAL_TEX_Y) ||
        (grid[gridY][gridX].wallTexX == DOOR_HORIZONTAL_TEX_X &&
         grid[gridY][gridX].wallTexY == DOOR_HORIZONTAL_TEX_Y)) {
        return;
    }

    // Get adjacent wall info
    bool hasNorth = (gridY > 0) && grid[gridY-1][gridX].hasWall;
    bool hasSouth = (gridY < GRID_SIZE-1) && grid[gridY+1][gridX].hasWall;
    bool hasEast = (gridX < GRID_SIZE-1) && grid[gridY][gridX+1].hasWall;
    bool hasWest = (gridX > 0) && grid[gridY][gridX-1].hasWall;

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
        return false; // Cancel the build action
    }

    // Proceed with placing the structure
    grid[gridY][gridX].hasWall = true;
    grid[gridY][gridX].isWalkable = false;

    switch(type) {
        case STRUCTURE_WALL:
            updateSurroundingStructures(gridX, gridY);
            break;

        case STRUCTURE_DOOR: {
            // Determine door orientation
            bool hasNorth = (gridY > 0) && grid[gridY-1][gridX].hasWall;
            bool hasSouth = (gridY < GRID_SIZE-1) && grid[gridY+1][gridX].hasWall;

            if (hasNorth || hasSouth) {
                grid[gridY][gridX].wallTexX = DOOR_VERTICAL_TEX_X;
                grid[gridY][gridX].wallTexY = DOOR_VERTICAL_TEX_Y;
            } else {
                grid[gridY][gridX].wallTexX = DOOR_HORIZONTAL_TEX_X;
                grid[gridY][gridX].wallTexY = DOOR_HORIZONTAL_TEX_Y;
            }
            
            // Initialize door as closed and unwalkable
            grid[gridY][gridX].isDoorOpen = false;
            grid[gridY][gridX].isWalkable = false;

            updateSurroundingStructures(gridX, gridY);
            break;
        }

        default:
            return false;
    }

    // After successful placement, check for enclosures
    Enclosure enclosure = detectEnclosure(gridX, gridY);
    if (enclosure.isValid) {
        EnclosureData newEnclosure = {0};  // Initialize to zero
        
        // Convert boundary tiles to Points for hashing
        Point* boundaryPoints = malloc(enclosure.tileCount * sizeof(Point));
        int wallCount = 0;
        int doorCount = 0;
        
        // Calculate center point and count structure types
        int sumX = 0, sumY = 0;
        for (int i = 0; i < enclosure.tileCount; i++) {
            int tileX = enclosure.tiles[i] % GRID_SIZE;
            int tileY = enclosure.tiles[i] / GRID_SIZE;
            
            boundaryPoints[i].x = tileX;
            boundaryPoints[i].y = tileY;
            
            sumX += tileX;
            sumY += tileY;
            
            // Count walls and doors
            if (grid[tileY][tileX].hasWall) {
                bool isDoor = (
                    // Check closed door textures
                    (grid[tileY][tileX].wallTexX == DOOR_VERTICAL_TEX_X && 
                    grid[tileY][tileX].wallTexY == DOOR_VERTICAL_TEX_Y) ||
                    (grid[tileY][tileX].wallTexX == DOOR_HORIZONTAL_TEX_X &&
                    grid[tileY][tileX].wallTexY == DOOR_HORIZONTAL_TEX_Y) ||
                    // Check open door textures
                    (grid[tileY][tileX].wallTexX == DOOR_VERTICAL_OPEN_TEX_X && 
                    grid[tileY][tileX].wallTexY == DOOR_VERTICAL_OPEN_TEX_Y) ||
                    (grid[tileY][tileX].wallTexX == DOOR_HORIZONTAL_OPEN_TEX_X &&
                    grid[tileY][tileX].wallTexY == DOOR_HORIZONTAL_OPEN_TEX_Y)
                );

                if (isDoor) {
                    doorCount++;
                } else {
                    wallCount++;
                }
            }
        }

        // Calculate hash
        newEnclosure.hash = calculateEnclosureHash(boundaryPoints, enclosure.tileCount, enclosure.tileCount);
        
        // Store enclosure data
        newEnclosure.boundaryTiles = boundaryPoints;
        newEnclosure.boundaryCount = enclosure.tileCount;
        newEnclosure.totalArea = enclosure.tileCount;  // Basic area calculation for now
        newEnclosure.centerPoint.x = sumX / enclosure.tileCount;
        newEnclosure.centerPoint.y = sumY / enclosure.tileCount;
        newEnclosure.doorCount = doorCount;
        newEnclosure.wallCount = wallCount;
        newEnclosure.isValid = true;

        // Add to enclosure manager
        addEnclosure(&globalEnclosureManager, &newEnclosure);

        printf("Found enclosure with %d tiles! Hash: %" PRIu64 "\n", 
            enclosure.tileCount, newEnclosure.hash);
        printf("Center point: (%d, %d)\n", 
               newEnclosure.centerPoint.x, newEnclosure.centerPoint.y);
        printf("Walls: %d, Doors: %d\n", wallCount, doorCount);

        // Debug print the enclosure path
        for (int i = 0; i < enclosure.tileCount; i++) {
            int tileX = enclosure.tiles[i] % GRID_SIZE;
            int tileY = enclosure.tiles[i] / GRID_SIZE;
            printf("Enclosure tile %d: (%d, %d)\n", i, tileX, tileY);
        }

        free(enclosure.tiles);  // Free the original tiles array
    } else {
        printf("No enclosure found from placement at (%d, %d)\n", gridX, gridY);
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
        mode->currentType = (mode->currentType + 1) % STRUCTURE_COUNT;
    } else {
        // Add STRUCTURE_COUNT before subtraction to handle negative values
        mode->currentType = (mode->currentType - 1 + STRUCTURE_COUNT) % STRUCTURE_COUNT;
    }

    // Skip STRUCTURE_NONE in the cycle
    if (mode->currentType == STRUCTURE_NONE) {
        mode->currentType = forward ? STRUCTURE_WALL : STRUCTURE_DOOR;
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
                (!requireWalkable || grid[checkY][checkX].isWalkable)) {
                
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
    // Verify it's actually a door by checking texture coordinates for both closed and open states
    bool isDoor = (
        // Check closed door textures
        (grid[gridY][gridX].wallTexX == DOOR_VERTICAL_TEX_X && 
         grid[gridY][gridX].wallTexY == DOOR_VERTICAL_TEX_Y) ||
        (grid[gridY][gridX].wallTexX == DOOR_HORIZONTAL_TEX_X &&
         grid[gridY][gridX].wallTexY == DOOR_HORIZONTAL_TEX_Y) ||
        // Check open door textures
        (grid[gridY][gridX].wallTexX == DOOR_VERTICAL_OPEN_TEX_X && 
         grid[gridY][gridX].wallTexY == DOOR_VERTICAL_OPEN_TEX_Y) ||
        (grid[gridY][gridX].wallTexX == DOOR_HORIZONTAL_OPEN_TEX_X &&
         grid[gridY][gridX].wallTexY == DOOR_HORIZONTAL_OPEN_TEX_Y)
    );

    if (!isDoor) return false;

    bool isNearby = (
        abs(gridX - player->entity.gridX) <= 1 && 
        abs(gridY - player->entity.gridY) <= 1
    );

    if (isNearby) {
        // Toggle door state
        grid[gridY][gridX].isDoorOpen = !grid[gridY][gridX].isDoorOpen;
        grid[gridY][gridX].isWalkable = grid[gridY][gridX].isDoorOpen;
        
        bool isVertical = (
            (grid[gridY][gridX].wallTexX == DOOR_VERTICAL_TEX_X && 
             grid[gridY][gridX].wallTexY == DOOR_VERTICAL_TEX_Y) ||
            (grid[gridY][gridX].wallTexX == DOOR_VERTICAL_OPEN_TEX_X && 
             grid[gridY][gridX].wallTexY == DOOR_VERTICAL_OPEN_TEX_Y)
        );

        printf("is this vertical? \n%d\n", isVertical);
        // Then toggle between closed and open textures while maintaining orientation
        if (isVertical) {
            if (grid[gridY][gridX].isDoorOpen) {
                grid[gridY][gridX].wallTexX = DOOR_VERTICAL_OPEN_TEX_X;
                grid[gridY][gridX].wallTexY = DOOR_VERTICAL_OPEN_TEX_Y;
            } else {
                grid[gridY][gridX].wallTexX = DOOR_VERTICAL_TEX_X;
                grid[gridY][gridX].wallTexY = DOOR_VERTICAL_TEX_Y;
            }
        } else {
            if (grid[gridY][gridX].isDoorOpen) {
                grid[gridY][gridX].wallTexX = DOOR_HORIZONTAL_OPEN_TEX_X;
                grid[gridY][gridX].wallTexY = DOOR_HORIZONTAL_OPEN_TEX_Y;
            } else {
                grid[gridY][gridX].wallTexX = DOOR_HORIZONTAL_TEX_X;
                grid[gridY][gridX].wallTexY = DOOR_HORIZONTAL_TEX_Y;
            }
        }
        
        return true;
    } else {
        // Path to nearest door-adjacent tile
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
    return grid[y][x].hasWall;
}

Enclosure detectEnclosure(int startX, int startY) {
    Enclosure result = {NULL, 0, false, 0};  // Initialize all fields including hash
    
    // Keep the initial cycle detection lean
    bool* visited = calloc(GRID_SIZE * GRID_SIZE, sizeof(bool));
    int* wallPath = malloc(GRID_SIZE * GRID_SIZE * sizeof(int));
    int pathLength = 0;
    
    // Stack for DFS
    typedef struct {
        int x, y;
        int prevX, prevY;
        int direction;
    } PathNode;
    
    PathNode* stack = malloc(GRID_SIZE * GRID_SIZE * sizeof(PathNode));
    int stackSize = 0;
    
    stack[stackSize++] = (PathNode){startX, startY, -1, -1, -1};
    
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
                stack[stackSize++] = (PathNode){newX, newY, current.x, current.y, i};
            }
        }
    }

    // Only if we found a cycle do we perform the expensive calculations
    if (foundCycle) {
        // Convert the path indices to Point structures for hashing
        Point* boundaryPoints = malloc(pathLength * sizeof(Point));
        for (int i = 0; i < pathLength; i++) {
            boundaryPoints[i].x = wallPath[i] % GRID_SIZE;
            boundaryPoints[i].y = wallPath[i] / GRID_SIZE;
        }

        // Basic area calculation (can be refined later)
        int area = pathLength;  // For now just using boundary length

        // Calculate hash
        result.hash = calculateEnclosureHash(boundaryPoints, pathLength, area);

        // Store in result
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