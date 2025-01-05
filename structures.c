#include "structures.h"
#include <stdio.h>
#include <math.h>
#include "grid.h"
#include "gameloop.h"
#include "player.h"
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