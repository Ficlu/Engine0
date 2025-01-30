#ifndef GAMELOOP_H
#define GAMELOOP_H

#include <stdbool.h>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include "grid.h"
#include "player.h"
#include "enemy.h"
#include <stdatomic.h>
#include "pathfinding.h"
#include "asciiMap.h"  // Add this line
#include "rendering.h"
#include "ui.h"
#define GAME_VIEW_WIDTH 800
#define SIDEBAR_WIDTH 300
#define WINDOW_WIDTH (GAME_VIEW_WIDTH + SIDEBAR_WIDTH)
#define WINDOW_HEIGHT 800
#define MAX_ENEMIES 80
#define MOVE_SPEED 0.0005f
#define GAME_LOGIC_INTERVAL_MS ((Uint32)600)
#define CAMERA_ZOOM 2.00f  
#define TILE_SIZE (1.0f / GRID_SIZE)
#define MAX_ENTITIES (MAX_ENEMIES + 1)
#define PHYSICS_INTERVAL_MS 12



extern GLuint outlineVAO;
extern GLuint outlineVBO;
extern GLuint tilesBatchVAO;
extern GLuint tilesBatchVBO;
extern Entity* allEntities[MAX_ENTITIES];
extern Uint32 FRAME_TIME_MS;
bool LoadGame(const char* filename);


void drawTargetTileOutline(int x, int y, float cameraOffsetX, float cameraOffsetY, float zoomFactor);
void InitializeEngine(void);
void InitializeGameState(bool isNewGame);
void initializeTilesBatchVAO();
float lerp(float a, float b, float t);
void WorldToScreenCoords(int gridX, int gridY, float cameraOffsetX, float cameraOffsetY, float zoomFactor, float* screenX, float* screenY);
void setGridSize(int size);
void CleanupEntities(void);
void GameLoop();
void Initialize();
void HandleInput();
void UpdateGameLogic();
void UpdateEnemy(Enemy* enemy, Entity** allEntities, int entityCount, Uint32 currentTime);
void Render();
int PhysicsLoop(void* arg);
void CleanUp();
void RenderTiles(float cameraOffsetX, float cameraOffsetY, float zoomFactor);
void RenderEntities(float cameraOffsetX, float cameraOffsetY, float zoomFactor);
void generateTerrain();
void drawTargetTileOutline(int x, int y, float cameraOffsetX, float cameraOffsetY, float zoomFactor);
bool westIsCorner(int x, int y);
bool eastIsCorner(int x, int y);
void updateWallTextures(int x, int y);  // Add this declaration
bool isTallStructureVisible(float worldX, float worldY, float heightOffset, float cameraOffsetX, float cameraOffsetY, float zoomFactor);
void RenderTreeCanopies(float cameraOffsetX, float cameraOffsetY, float zoomFactor);
#endif // GAMELOOP_H