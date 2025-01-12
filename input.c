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

static void HandleSidebarClick(int mouseX, int mouseY) {
    // Convert window coordinates to sidebar-local coordinates
    int localX = mouseX - GAME_VIEW_WIDTH;
    int localY = mouseY;
    
    // Test button region (for now, let's say top 100 pixels of sidebar)
    if (localY < 100) {
        printf("Test button clicked in sidebar!\n");
        // Add your button handling logic here
    }
}
void HandleInput() {
    SDL_Event event;
   
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            atomic_store(&isRunning, false);
        } 
        else if (event.type == SDL_KEYDOWN) {
            printf("Key pressed: %d\n", event.key.keysym.sym);
           
            // Handle save/load first (works in any mode)
            if (event.key.keysym.mod & KMOD_CTRL) {
                switch (event.key.keysym.sym) {
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
           
            // Handle mode toggles
            switch(event.key.keysym.sym) {
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
        else if (event.type == SDL_MOUSEBUTTONDOWN) {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            if (isPointInGameView(mouseX, mouseY)) {
                float ndcX = (2.0f * mouseX / GAME_VIEW_WIDTH - 1.0f) / player.zoomFactor;
                float ndcY = (1.0f - 2.0f * mouseY / WINDOW_HEIGHT) / player.zoomFactor;

                float worldX = ndcX + player.cameraCurrentX;
                float worldY = ndcY + player.cameraCurrentY;

                int gridX = (int)((worldX + 1.0f) * GRID_SIZE / 2);
                int gridY = (int)((1.0f - worldY) * GRID_SIZE / 2);

                if (gridX >= 0 && gridX < GRID_SIZE && gridY >= 0 && gridY < GRID_SIZE) {
                    printf("Click detected at grid: (%d, %d)\n", gridX, gridY);
                    printf("Structure type at clicked location: %d\n", grid[gridY][gridX].structureType);

                    if (!placementMode.active) {
                        // Check for fern first
                        if (grid[gridY][gridX].structureType == STRUCTURE_PLANT &&
                            grid[gridY][gridX].materialType == MATERIAL_FERN) {
                            
                            printf("Attempting to harvest fern at (%d, %d)\n", gridX, gridY);
                            
                            // Check if player is close enough to harvest
                            int playerGridX = player.entity.gridX;
                            int playerGridY = player.entity.gridY;
                            bool isNearby = (
                                abs(gridX - playerGridX) <= 1 && 
                                abs(gridY - playerGridY) <= 1
                            );

                            if (isNearby) {
                                // Create new fern item
                                Item* fernItem = CreateItem(ITEM_FERN);
                                if (!fernItem) {
                                    printf("Failed to create fern item\n");
                                    return;
                                }
                                
                                // Try to add to inventory
                                if (AddItem(player.inventory, fernItem)) {
                                    printf("Fern added to inventory successfully\n");
                                    
                                    // Clear the grid cell
                                    grid[gridY][gridX].structureType = STRUCTURE_NONE;
                                    grid[gridY][gridX].materialType = MATERIAL_NONE;
                                    GRIDCELL_SET_WALKABLE(grid[gridY][gridX], true);
                                    
                                    printf("Grid cell cleared after harvesting\n");
                                } else {
                                    printf("Inventory full - couldn't add fern\n");
                                    DestroyItem(fernItem); // Clean up if we couldn't add it
                                }
                            } else {
                                // If not nearby, pathfind to the fern
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
                                    printf("Pathfinding to fern at (%d, %d)\n", gridX, gridY);
                                }
                            }
                        }
                        // Then check for door
                        else if (grid[gridY][gridX].structureType == STRUCTURE_DOOR) {
                            printf("Door clicked, attempting toggle\n");
                            toggleDoor(gridX, gridY, &player);
                        }
                        // Finally handle movement
                        else if (GRIDCELL_IS_WALKABLE(grid[gridY][gridX])) {
                            player.entity.finalGoalX = gridX;
                            player.entity.finalGoalY = gridY;
                            player.entity.targetGridX = player.entity.gridX;
                            player.entity.targetGridY = player.entity.gridY;
                            player.entity.needsPathfinding = true;
                            printf("Player final goal set: gridX = %d, gridY = %d\n", gridX, gridY);
                        }
                    }
                    else if (event.button.button == SDL_BUTTON_LEFT && placementMode.active) {
                        int playerGridX = player.entity.gridX;
                        int playerGridY = player.entity.gridY;
                        bool isNearby = (
                            abs(gridX - playerGridX) <= 1 && 
                            abs(gridY - playerGridY) <= 1
                        );

                        if (isNearby) {
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
                    }
                    else if (event.button.button == SDL_BUTTON_RIGHT && placementMode.active) {
                        if (grid[gridY][gridX].structureType == STRUCTURE_WALL || 
                            grid[gridY][gridX].structureType == STRUCTURE_DOOR) {
                            int playerGridX = player.entity.gridX;
                            int playerGridY = player.entity.gridY;
                            bool isNearby = (
                                abs(gridX - playerGridX) <= 1 && 
                                abs(gridY - playerGridY) <= 1
                            );

                            if (isNearby) {
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
            } else if (isPointInSidebar(mouseX, mouseY)) {
                HandleSidebarClick(mouseX, mouseY);
            }
        }
        else if (event.type == SDL_MOUSEMOTION && placementMode.active) {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            if (isPointInGameView(mouseX, mouseY)) {
                float ndcX = (2.0f * mouseX / GAME_VIEW_WIDTH - 1.0f) / player.zoomFactor;
                float ndcY = (1.0f - 2.0f * mouseY / WINDOW_HEIGHT) / player.zoomFactor;

                float worldX = ndcX + player.cameraCurrentX;
                float worldY = ndcY + player.cameraCurrentY;

                placementMode.previewX = (int)((worldX + 1.0f) * GRID_SIZE / 2);
                placementMode.previewY = (int)((1.0f - worldY) * GRID_SIZE / 2);
            }
        }
        else if (event.type == SDL_MOUSEWHEEL) {
            float zoomSpeed = 0.2f;
            float minZoom = 0.2f;
            float maxZoom = 20.0f;

            if (event.wheel.y > 0) {
                player.zoomFactor = fminf(player.zoomFactor + zoomSpeed, maxZoom);
            } else if (event.wheel.y < 0) {
                player.zoomFactor = fmaxf(player.zoomFactor - zoomSpeed, minZoom);
            }

            printf("Zoom factor: %.2f\n", player.zoomFactor);
        }
    }
}