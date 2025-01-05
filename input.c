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


// Declare external variables
extern atomic_bool isRunning;
extern Player player;
extern PlacementMode placementMode;

void HandleInput() {
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            atomic_store(&isRunning, false);
        } 
        else if (event.type == SDL_KEYDOWN) {
            printf("Key pressed: %d\n", event.key.keysym.sym);
            if (event.key.keysym.sym == SDLK_e) {
                placementMode.active = !placementMode.active;
                printf("Placement mode %s\n", placementMode.active ? "activated" : "deactivated");
            }
            else if (placementMode.active) {
                switch (event.key.keysym.sym) {
                    case SDLK_RIGHT:
                        cycleStructureType(&placementMode, true);
                        break;
                    case SDLK_LEFT:
                        cycleStructureType(&placementMode, false);
                        break;
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
        else if (event.type == SDL_MOUSEBUTTONDOWN) {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);

            float ndcX = (2.0f * mouseX / WINDOW_WIDTH - 1.0f) / player.zoomFactor;
            float ndcY = (1.0f - 2.0f * mouseY / WINDOW_HEIGHT) / player.zoomFactor;

            float worldX = ndcX + player.cameraCurrentX;
            float worldY = ndcY + player.cameraCurrentY;

            int gridX = (int)((worldX + 1.0f) * GRID_SIZE / 2);
            int gridY = (int)((1.0f - worldY) * GRID_SIZE / 2);

            if (gridX >= 0 && gridX < GRID_SIZE && gridY >= 0 && gridY < GRID_SIZE) {
                if (placementMode.active) {
                    if (event.button.button == SDL_BUTTON_LEFT) {
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
                            if (!placed) {
                                printf("Current structure type: %s\n", getStructureName(placementMode.currentType));
                                printf("Grid cell state: hasWall=%d, isWalkable=%d\n", 
                                    grid[gridY][gridX].hasWall, 
                                    grid[gridY][gridX].isWalkable);
                            }
                        } else {
                            // Find closest adjacent tile to target
                            int adjacentX = -1;
                            int adjacentY = -1;
                            float shortestDistance = INFINITY;
                            
                            // Check all adjacent tiles to the clicked position
                            for (int dy = -1; dy <= 1; dy++) {
                                for (int dx = -1; dx <= 1; dx++) {
                                    if (dx == 0 && dy == 0) continue;
                                    
                                    int checkX = gridX + dx;
                                    int checkY = gridY + dy;
                                    
                                    // Ensure tile is in bounds and walkable
                                    if (checkX >= 0 && checkX < GRID_SIZE && 
                                        checkY >= 0 && checkY < GRID_SIZE && 
                                        grid[checkY][checkX].isWalkable) {
                                        
                                        // Calculate distance from player to this adjacent tile
                                        float dist = sqrtf(powf(player.entity.gridX - checkX, 2) + 
                                                         powf(player.entity.gridY - checkY, 2));
                                        
                                        if (dist < shortestDistance) {
                                            shortestDistance = dist;
                                            adjacentX = checkX;
                                            adjacentY = checkY;
                                        }
                                    }
                                }
                            }
                            
                            // If we found a valid adjacent tile, path to it
                            if (adjacentX != -1 && adjacentY != -1) {
                                player.entity.finalGoalX = adjacentX;
                                player.entity.finalGoalY = adjacentY;
                                player.entity.targetGridX = player.entity.gridX;
                                player.entity.targetGridY = player.entity.gridY;
                                player.entity.needsPathfinding = true;
                                player.targetBuildX = gridX;
                                player.targetBuildY = gridY;
                                player.hasBuildTarget = true;
                                player.pendingBuildType = placementMode.currentType;
                                printf("Moving to adjacent tile (%d, %d) to build at (%d, %d)\n", 
                                       adjacentX, adjacentY, gridX, gridY);
                            }
                        }
                    }
                    else if (event.button.button == SDL_BUTTON_RIGHT) {
                        if (grid[gridY][gridX].hasWall) {
                            int playerGridX = player.entity.gridX;
                            int playerGridY = player.entity.gridY;
                            bool isNearby = (
                                abs(gridX - playerGridX) <= 1 && 
                                abs(gridY - playerGridY) <= 1
                            );

                            if (isNearby) {
                                grid[gridY][gridX].hasWall = false;
                                grid[gridY][gridX].isWalkable = true;
                                updateSurroundingStructures(gridX, gridY);
                                printf("Removed structure at grid position: %d, %d\n", gridX, gridY);
                            } else {
                                printf("Can't remove structure - too far away\n");
                            }
                        }
                    }
                }
                else if (grid[gridY][gridX].isWalkable) {
                    player.entity.finalGoalX = gridX;
                    player.entity.finalGoalY = gridY;
                    player.entity.targetGridX = player.entity.gridX;
                    player.entity.targetGridY = player.entity.gridY;
                    player.entity.needsPathfinding = true;
                    printf("Player final goal set: gridX = %d, gridY = %d\n", gridX, gridY);
                }
            }
        }
        else if (event.type == SDL_MOUSEMOTION && placementMode.active) {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            
            float ndcX = (2.0f * mouseX / WINDOW_WIDTH - 1.0f) / player.zoomFactor;
            float ndcY = (1.0f - 2.0f * mouseY / WINDOW_HEIGHT) / player.zoomFactor;

            float worldX = ndcX + player.cameraCurrentX;
            float worldY = ndcY + player.cameraCurrentY;

            placementMode.previewX = (int)((worldX + 1.0f) * GRID_SIZE / 2);
            placementMode.previewY = (int)((1.0f - worldY) * GRID_SIZE / 2);
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