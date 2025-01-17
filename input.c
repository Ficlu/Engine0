// input.c
#include "input.h"
#include <stdio.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <stdatomic.h>
#include <stdbool.h>
#include "grid.h"
#include "player.h"
#include "structures.h"
#include "saveload.h"
#include "gameloop.h"
#include "ui.h"
#include "rendering.h"

// Declare external variables
extern atomic_bool isRunning;
extern Player player;
extern PlacementMode placementMode;
extern UIState uiState;

// Convert window coordinates to grid coordinates
GridCoordinates WindowToGridCoordinates(int mouseX, int mouseY, float cameraX, float cameraY, float zoomFactor) {
    float ndcX = (2.0f * mouseX / GAME_VIEW_WIDTH - 1.0f) / zoomFactor;
    float ndcY = (1.0f - 2.0f * mouseY / WINDOW_HEIGHT) / zoomFactor;

    float worldX = ndcX + cameraX;
    float worldY = ndcY + cameraY;

    GridCoordinates coords;
    coords.gridX = (int)((worldX + 1.0f) * GRID_SIZE / 2);
    coords.gridY = (int)((1.0f - worldY) * GRID_SIZE / 2);
    
    return coords;
}

// Check if target is within player's interaction range
bool IsWithinPlayerRange(int gridX, int gridY, int playerX, int playerY) {
    return (abs(gridX - playerX) <= 1 && abs(gridY - playerY) <= 1);
}

// Handle keyboard save/load commands
static void HandleSaveLoad(SDL_KeyboardEvent* key) {
    if (key->keysym.mod & KMOD_CTRL) {
        switch (key->keysym.sym) {
            case SDLK_s:
                if (saveGameState("game_save.sav")) {
                    printf("Game saved successfully!\n");
                } else {
                    printf("Failed to save game!\n");
                }
                break;
            case SDLK_l:
                if (loadGameState("game_save.sav")) {
                    printf("Game loaded successfully!\n");
                } else {
                    printf("Failed to load game!\n");
                }
                break;
        }
    }
}

// Handle mode toggles and placement controls
static void HandleModeToggles(SDL_KeyboardEvent* key) {
    switch(key->keysym.sym) {
        case SDLK_e:
            placementMode.active = !placementMode.active;
            printf("Placement mode %s\n", placementMode.active ? "activated" : "deactivated");
            break;
        case SDLK_i:
            uiState.inventoryOpen = !uiState.inventoryOpen;
            printf("Inventory %s\n", uiState.inventoryOpen ? "opened" : "closed");
            break;
        case SDLK_RIGHT:
            if (placementMode.active) {
                cycleStructureType(&placementMode, true);
            }
            break;
        case SDLK_LEFT:
            if (placementMode.active) {
                cycleStructureType(&placementMode, false);
            }
            break;
    }
}

// Handle harvesting interactions
static void HandleHarvesting(int gridX, int gridY) {
    printf("Attempting to harvest fern at (%d, %d)\n", gridX, gridY);
    
    if (IsWithinPlayerRange(gridX, gridY, player.entity.gridX, player.entity.gridY)) {
        Item* fernItem = CreateItem(ITEM_FERN);
        if (!fernItem) {
            printf("Failed to create fern item\n");
            return;
        }
        
        printf("Created fern item successfully, adding to inventory...\n");
        
        bool added = AddItem(player.inventory, fernItem);
        printf("AddItem result: %d\n", added);
        
        if (added) {
            awardForagingExp(&player, fernItem);
            grid[gridY][gridX].structureType = STRUCTURE_NONE;
            grid[gridY][gridX].materialType = MATERIAL_NONE;
            GRIDCELL_SET_WALKABLE(grid[gridY][gridX], true);
            printf("Grid cell cleared after successful harvest\n");
        } else {
            printf("Failed to add item to inventory - destroying item\n");
            DestroyItem(fernItem);
        }
    } else {
        AdjacentTile nearest = findNearestAdjacentTile(gridX, gridY,
                                                     player.entity.gridX,
                                                     player.entity.gridY,
                                                     true);
        if (nearest.x != -1) {
            player.entity.finalGoalX = nearest.x;
            player.entity.finalGoalY = nearest.y;
            player.entity.targetGridX = player.entity.gridX;
            player.entity.targetGridY = player.entity.gridY;
            player.entity.needsPathfinding = true;
            
            player.targetHarvestX = gridX;
            player.targetHarvestY = gridY;
            player.hasHarvestTarget = true;
            player.pendingHarvestType = grid[gridY][gridX].materialType;
            
            printf("Pathfinding to harvest fern at (%d, %d)\n", gridX, gridY);
        }
    }
}

// Handle building placement
static void HandlePlacement(int gridX, int gridY, Uint8 button) {
    if (button == SDL_BUTTON_LEFT) {
        if (IsWithinPlayerRange(gridX, gridY, player.entity.gridX, player.entity.gridY)) {
            printf("Attempting direct placement at (%d, %d)\n", gridX, gridY);
            bool placed = placeStructure(placementMode.currentType, gridX, gridY);
            printf("Direct placement result: %s\n", placed ? "success" : "failed");
        } else {
            AdjacentTile nearest = findNearestAdjacentTile(gridX, gridY,
                                                         player.entity.gridX,
                                                         player.entity.gridY,
                                                         true);
            if (nearest.x != -1) {
                int pathLength;
                Node* path = findPath(player.entity.gridX, player.entity.gridY, 
                                    nearest.x, nearest.y, &pathLength);
                
                if (path != NULL) {
                    player.entity.finalGoalX = nearest.x;
                    player.entity.finalGoalY = nearest.y;
                    player.entity.targetGridX = player.entity.gridX;
                    player.entity.targetGridY = player.entity.gridY;
                    player.entity.needsPathfinding = true;
                    player.targetBuildX = gridX;
                    player.targetBuildY = gridY;
                    player.hasBuildTarget = true;
                    player.pendingBuildType = placementMode.currentType;
                    free(path);
                }
            }
        }
    } else if (button == SDL_BUTTON_RIGHT) {
        if (grid[gridY][gridX].structureType == STRUCTURE_WALL || 
            grid[gridY][gridX].structureType == STRUCTURE_DOOR) {
            if (IsWithinPlayerRange(gridX, gridY, player.entity.gridX, player.entity.gridY)) {
                grid[gridY][gridX].structureType = STRUCTURE_NONE;  
                GRIDCELL_SET_WALKABLE(grid[gridY][gridX], true);
                updateSurroundingStructures(gridX, gridY);
                printf("Removed structure at grid position: %d, %d\n", gridX, gridY);
            } else {
                printf("Can't remove structure - too far away\n");
            }
        }
    }
}

// Handle player movement
static void HandleMovement(int gridX, int gridY) {
    player.hasHarvestTarget = false;
    player.pendingHarvestType = 0;
    
    player.entity.finalGoalX = gridX;
    player.entity.finalGoalY = gridY;
    player.entity.targetGridX = player.entity.gridX;
    player.entity.targetGridY = player.entity.gridY;
    player.entity.needsPathfinding = true;
    printf("Player final goal set: gridX = %d, gridY = %d\n", gridX, gridY);
}

static void HandleSidebarClick(int mouseX, int mouseY) {
    // Convert window coordinates to sidebar-local coordinates
    int localX = mouseX - GAME_VIEW_WIDTH;
    int localY = mouseY;
}

// input.c
// ... (previous code remains the same until HandleInput()) ...

static void HandleMouseInput(SDL_Event* event) {
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    switch (event->type) {
        case SDL_MOUSEBUTTONDOWN:
            if (isPointInGameView(mouseX, mouseY)) {
                GridCoordinates coords = WindowToGridCoordinates(mouseX, mouseY, 
                    player.cameraCurrentX, player.cameraCurrentY, player.zoomFactor);
                
                if (coords.gridX >= 0 && coords.gridX < GRID_SIZE && 
                    coords.gridY >= 0 && coords.gridY < GRID_SIZE) {
                    
                    if (!placementMode.active) {
                        switch (grid[coords.gridY][coords.gridX].structureType) {
                            case STRUCTURE_PLANT:
                                if (grid[coords.gridY][coords.gridX].materialType == MATERIAL_FERN) {
                                    HandleHarvesting(coords.gridX, coords.gridY);
                                }
                                break;
                            case STRUCTURE_DOOR:
                                printf("Door clicked, attempting toggle\n");
                                toggleDoor(coords.gridX, coords.gridY, &player);
                                break;
                            default:
                                if (GRIDCELL_IS_WALKABLE(grid[coords.gridY][coords.gridX])) {
                                    HandleMovement(coords.gridX, coords.gridY);
                                }
                                break;
                        }
                    } else {
                        HandlePlacement(coords.gridX, coords.gridY, event->button.button);
                    }
                }
            } else if (isPointInSidebar(mouseX, mouseY)) {
                HandleSidebarClick(mouseX, mouseY);
            }
            break;

        case SDL_MOUSEMOTION:
            if (placementMode.active && isPointInGameView(mouseX, mouseY)) {
                GridCoordinates coords = WindowToGridCoordinates(mouseX, mouseY,
                    player.cameraCurrentX, player.cameraCurrentY, player.zoomFactor);
                
                if (coords.gridX >= 0 && coords.gridX < GRID_SIZE && 
                    coords.gridY >= 0 && coords.gridY < GRID_SIZE) {
                    placementMode.previewX = coords.gridX;
                    placementMode.previewY = coords.gridY;
                }
            }
            break;

        case SDL_MOUSEWHEEL:
            HandleZoom(event->wheel.y);
            break;
    }
}

static void HandleZoom(int wheelDelta) {
    float zoomSpeed = 0.2f;
    float minZoom = 0.2f;
    float maxZoom = 20.0f;

    if (wheelDelta > 0) {
        player.zoomFactor = fminf(player.zoomFactor + zoomSpeed, maxZoom);
    } else if (wheelDelta < 0) {
        player.zoomFactor = fmaxf(player.zoomFactor - zoomSpeed, minZoom);
    }

    printf("Zoom factor: %.2f\n", player.zoomFactor);
}

void HandleInput() {
    SDL_Event event;
   
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                atomic_store(&isRunning, false);
                break;

            case SDL_KEYDOWN:
                printf("Key pressed: %d\n", event.key.keysym.sym);
                HandleSaveLoad(&event.key);
                HandleModeToggles(&event.key);
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEMOTION:
            case SDL_MOUSEWHEEL:
                HandleMouseInput(&event);
                break;

            default:
                break;
        }
    }
}