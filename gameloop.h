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

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define MAX_ENEMIES 150
#define MOVE_SPEED 0.001f
#define GAME_LOGIC_INTERVAL_MS ((Uint32)600)
#define CAMERA_ZOOM 5.00f  
#define TILE_SIZE (1.0f / GRID_SIZE)
#define MAX_ENTITIES (MAX_ENEMIES + 1)
#define PHYSICS_INTERVAL_MS 12
#define WALL_VERTICAL_TEX_X (0.0f / 3.0f)
#define WALL_VERTICAL_TEX_Y (1.0f / 4.0f)
#define WALL_FRONT_TEX_X (1.0f / 3.0f)
#define WALL_FRONT_TEX_Y (1.0f / 4.0f)
#define WALL_TOP_LEFT_TEX_X (2.0f / 3.0f)
#define WALL_TOP_LEFT_TEX_Y (1.0f / 4.0f)
#define WALL_BOTTOM_LEFT_TEX_X (0.0f / 3.0f)
#define WALL_BOTTOM_LEFT_TEX_Y (0.0f / 4.0f)
#define WALL_TOP_RIGHT_TEX_X (1.0f / 3.0f)
#define WALL_TOP_RIGHT_TEX_Y (0.0f / 4.0f)
#define WALL_BOTTOM_RIGHT_TEX_X (2.0f / 3.0f)
#define WALL_BOTTOM_RIGHT_TEX_Y (0.0f / 4.0f)
extern GLuint outlineVAO;
extern GLuint outlineVBO;
extern GLuint tilesBatchVAO;
extern GLuint tilesBatchVBO;
extern Entity* allEntities[MAX_ENTITIES];
extern Uint32 FRAME_TIME_MS;
void drawTargetTileOutline(int x, int y, float cameraOffsetX, float cameraOffsetY, float zoomFactor);

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
#endif // GAMELOOP_H