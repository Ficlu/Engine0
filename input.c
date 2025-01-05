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
           
           // Then handle placement mode toggle
           if (event.key.keysym.sym == SDLK_e) {
               placementMode.active = !placementMode.active;
               printf("Placement mode %s\n", placementMode.active ? "activated" : "deactivated");
           }
           // Then handle placement mode specific controls
           else if (placementMode.active) {
               switch (event.key.keysym.sym) {
                   case SDLK_RIGHT:
                       cycleStructureType(&placementMode, true);
                       break;
                   case SDLK_LEFT:
                       cycleStructureType(&placementMode, false);
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
                               player.targetBuildX = gridX;
                               player.targetBuildY = gridY;
                               player.hasBuildTarget = true;
                               player.pendingBuildType = placementMode.currentType;
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
               else {
                   if (event.button.button == SDL_BUTTON_LEFT) {
                       if (grid[gridY][gridX].hasWall) {
                           toggleDoor(gridX, gridY, &player);
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