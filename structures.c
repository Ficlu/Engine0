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

#define FNV_PRIME 1099511628211ULL
#define FNV_OFFSET 14695981039346656037ULL

EnclosureManager globalEnclosureManager;

/**
 * @brief Initializes the structure system.
 *
 * Sets up any necessary data or states for managing structures.
 */
void initializeStructureSystem(void) {
    printf("Structure system initialized\n");
}

/**
 * @brief Retrieves the name of a structure type.
 *
 * @param type The structure type to query.
 * @return A string representing the name of the structure.
 */
const char* getStructureName(StructureType type) {
    switch(type) {
        case STRUCTURE_WALL: return "Wall";
        case STRUCTURE_DOOR: return "Door";
        default: return "Unknown";
    }
}

/**
 * @brief Checks if a structure can be placed at the specified grid location.
 *
 * @param type The type of structure to place.
 * @param gridX The X-coordinate on the grid.
 * @param gridY The Y-coordinate on the grid.
 * @return `true` if the structure can be placed; otherwise, `false`.
 */
bool canPlaceStructure(StructureType type, int gridX, int gridY) {
    // Basic bounds checking
    if (gridX < 0 || gridX >= GRID_SIZE || gridY < 0 || gridY >= GRID_SIZE) {
        printf("Out of bounds\n");
        return false;
    }

    // Check if tile is already occupied
    if (grid[gridY][gridX].structureType != STRUCTURE_NONE) {
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

        case STRUCTURE_PLANT:
            // Plants can only be placed on empty walkable tiles
            return GRIDCELL_IS_WALKABLE(grid[gridY][gridX]);

        default:
            return false;
    }
}

/**
 * @brief Updates wall textures based on surrounding structures.
 *
 * @param gridX The X-coordinate of the wall.
 * @param gridY The Y-coordinate of the wall.
 *
 * Adjusts the texture coordinates for a wall tile to reflect its surroundings.
 */
void updateWallTextures(int gridX, int gridY) {
    // Skip if not a wall/door
    if (!isWallOrDoor(gridX, gridY)) return;

    // Preserve door textures
    if ((grid[gridY][gridX].wallTexX == DOOR_VERTICAL_TEX_X && 
         grid[gridY][gridX].wallTexY == DOOR_VERTICAL_TEX_Y) ||
        (grid[gridY][gridX].wallTexX == DOOR_HORIZONTAL_TEX_X &&
         grid[gridY][gridX].wallTexY == DOOR_HORIZONTAL_TEX_Y)) {
        return;
    }

    // Get adjacent wall info - explicitly check for walls/doors only
    bool hasNorth = (gridY > 0) && isWallOrDoor(gridX, gridY-1);
    bool hasSouth = (gridY < GRID_SIZE-1) && isWallOrDoor(gridX, gridY+1);
    bool hasEast = (gridX < GRID_SIZE-1) && isWallOrDoor(gridX+1, gridY);
    bool hasWest = (gridX > 0) && isWallOrDoor(gridX-1, gridY);

    // Rest of the texture logic remains the same...
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
    else if (hasNorth || hasSouth) {
        grid[gridY][gridX].wallTexX = WALL_VERTICAL_TEX_X;
        grid[gridY][gridX].wallTexY = WALL_VERTICAL_TEX_Y;
    } 
    else {
        grid[gridY][gridX].wallTexX = WALL_FRONT_TEX_X;
        grid[gridY][gridX].wallTexY = WALL_FRONT_TEX_Y;
    }
}

/**
 * @brief Updates surrounding structures after a placement.
 *
 * @param gridX The X-coordinate of the placed structure.
 * @param gridY The Y-coordinate of the placed structure.
 *
 * Ensures adjacent walls and tiles have updated textures and properties.
 */
void updateSurroundingStructures(int gridX, int gridY) {
    // Only update if center tile is a wall/door
    if (!isWallOrDoor(gridX, gridY)) return;

    // Update the placed structure and all adjacent cells
    if (gridY > 0 && isWallOrDoor(gridX, gridY-1)) 
        updateWallTextures(gridX, gridY-1);
    if (gridY < GRID_SIZE-1 && isWallOrDoor(gridX, gridY+1)) 
        updateWallTextures(gridX, gridY+1);
    if (gridX > 0 && isWallOrDoor(gridX-1, gridY)) 
        updateWallTextures(gridX-1, gridY);
    if (gridX < GRID_SIZE-1 && isWallOrDoor(gridX+1, gridY)) 
        updateWallTextures(gridX+1, gridY);
    updateWallTextures(gridX, gridY);
}

/**
 * @brief Places a structure at the specified grid location.
 *
 * @param type The type of structure to place.
 * @param gridX The X-coordinate on the grid.
 * @param gridY The Y-coordinate on the grid.
 * @return `true` if the structure was placed successfully; otherwise, `false`.
 */
bool placeStructure(StructureType type, int gridX, int gridY) {
    if (!canPlaceStructure(type, gridX, gridY)) {
        return false;
    }

    // Check if an entity is targeting the tile
    if (isEntityTargetingTile(gridX, gridY)) {
        printf("Cannot place structure at (%d, %d): entity is targeting this tile.\n", gridX, gridY);
        return false;
    }

    // Set the structure type
    grid[gridY][gridX].structureType = type;
    grid[gridY][gridX].materialType = MATERIAL_WOOD; // Default to wood for walls/doors

    switch(type) {
        case STRUCTURE_WALL:
            GRIDCELL_SET_WALKABLE(grid[gridY][gridX], false);
            updateSurroundingStructures(gridX, gridY);
            break;
            
        case STRUCTURE_DOOR: {
            GRIDCELL_SET_WALKABLE(grid[gridY][gridX], false);
            // Determine door orientation based on adjacent walls
            bool hasNorth = (gridY > 0) && isWallOrDoor(gridX, gridY-1);
            bool hasSouth = (gridY < GRID_SIZE-1) && isWallOrDoor(gridX, gridY+1);

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
            GRIDCELL_SET_WALKABLE(grid[gridY][gridX], false);
            grid[gridY][gridX].materialType = MATERIAL_FERN; // Default plant type
            printf("Placed plant: structureType=%d, materialType=%d\n", 
                   grid[gridY][gridX].structureType, 
                   grid[gridY][gridX].materialType);
            return true; // Return early for plants - skip enclosure detection
            break;

        default:
            return false;
    }

    // Only check for enclosures if we placed a wall or door
    if (type == STRUCTURE_WALL || type == STRUCTURE_DOOR) {
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
                
                if (isWallOrDoor(tileX, tileY)) {
                    if (grid[tileY][tileX].structureType == STRUCTURE_DOOR) {
                        doorCount++;
                    } else {
                        wallCount++;
                    }
                }
            }

            // Rest of enclosure handling code remains the same...
            // [Previous enclosure processing code here]
        }
    }

    printf("Placed %s at grid position: %d, %d\n", getStructureName(type), gridX, gridY);
    return true;
}

/**
 * @brief Cleans up the structure system.
 *
 * Releases resources and resets the structure system to an uninitialized state.
 */
void cleanupStructureSystem(void) {
    printf("Structure system cleaned up\n");
}

/**
 * @brief Cycles through structure types in placement mode.
 *
 * @param mode The current placement mode.
 * @param forward If `true`, cycles forward; otherwise, cycles backward.
 */
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

/**
 * @brief Checks if any entity is targeting a specific tile.
 *
 * @param gridX The X-coordinate of the tile.
 * @param gridY The Y-coordinate of the tile.
 * @return `true` if an entity is targeting the tile; otherwise, `false`.
 */
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

/**
 * @brief Finds the nearest walkable tile adjacent to a target.
 *
 * @param targetX The X-coordinate of the target tile.
 * @param targetY The Y-coordinate of the target tile.
 * @param fromX The X-coordinate of the starting position.
 * @param fromY The Y-coordinate of the starting position.
 * @param requireWalkable If `true`, only considers walkable tiles.
 * @return The nearest adjacent tile and its distance.
 */
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

/**
 * @brief Toggles the open or closed state of a door.
 *
 * @param gridX The X-coordinate of the door.
 * @param gridY The Y-coordinate of the door.
 * @param player A pointer to the player interacting with the door.
 * @return `true` if the door was toggled successfully; otherwise, `false`.
 */
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

/**
 * @brief Checks if a tile contains a wall or door.
 *
 * @param x The X-coordinate of the tile.
 * @param y The Y-coordinate of the tile.
 * @return `true` if the tile contains a wall or door; otherwise, `false`.
 */
bool isWallOrDoor(int x, int y) {
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) return false;
    return (grid[y][x].structureType == STRUCTURE_WALL || 
            grid[y][x].structureType == STRUCTURE_DOOR);
}

/**
 * @brief Detects an enclosure starting from a specific tile.
 *
 * @param startX The X-coordinate of the starting tile.
 * @param startY The Y-coordinate of the starting tile.
 * @return An enclosure structure containing its tiles and properties.
 */
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

/**
 * @brief Calculates a hash for an enclosure based on its boundary.
 *
 * @param boundaryTiles The boundary tiles of the enclosure.
 * @param boundaryCount The number of boundary tiles.
 * @param totalArea The total area of the enclosure.
 * @return A unique hash for the enclosure.
 */
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

/**
 * @brief Initializes the enclosure manager.
 *
 * @param manager A pointer to the enclosure manager to initialize.
 */
void initEnclosureManager(EnclosureManager* manager) {
    manager->capacity = 16;  // Initial capacity
    manager->count = 0;
    manager->enclosures = malloc(manager->capacity * sizeof(EnclosureData));
    if (!manager->enclosures) {
        fprintf(stderr, "Failed to initialize enclosure manager\n");
        exit(1);
    }
}

/**
 * @brief Adds a new enclosure to the manager.
 *
 * @param manager A pointer to the enclosure manager.
 * @param enclosure The enclosure data to add.
 */
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

/**
 * @brief Finds an enclosure by its hash.
 *
 * @param manager A pointer to the enclosure manager.
 * @param hash The hash of the enclosure to find.
 * @return A pointer to the found enclosure or `NULL` if not found.
 */
EnclosureData* findEnclosure(EnclosureManager* manager, uint64_t hash) {
    for (int i = 0; i < manager->count; i++) {
        if (manager->enclosures[i].hash == hash) {
            return &manager->enclosures[i];
        }
    }
    return NULL;
}

/**
 * @brief Removes an enclosure from the manager by its hash.
 *
 * @param manager A pointer to the enclosure manager.
 * @param hash The hash of the enclosure to remove.
 */
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

/**
 * @brief Cleans up the enclosure manager.
 *
 * Releases all resources associated with the enclosures managed.
 *
 * @param manager A pointer to the enclosure manager to clean up.
 */
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

