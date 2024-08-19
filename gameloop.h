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

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define MAX_ENEMIES 200
#define MOVE_SPEED 0.002f
#define GAME_LOGIC_INTERVAL_MS 600
#define FRAME_TIME_MS 24
#define CAMERA_ZOOM 4.00f  
#define TILE_SIZE (1.0f / GRID_SIZE)
#define MAX_ENTITIES (MAX_ENEMIES + 1)

extern GLuint outlineVAO;
extern Entity* allEntities[MAX_ENTITIES];

float lerp(float a, float b, float t);
void WorldToScreenCoords(int gridX, int gridY, float cameraOffsetX, float cameraOffsetY, float zoomFactor, float* screenX, float* screenY);
void setGridSize(int size);
void CleanupEntities(void);
void GameLoop();
void Initialize();
void HandleInput();
void UpdateGameLogic();
void Render();
int PhysicsLoop(void* arg);
void CleanUp();
void RenderTiles(float cameraOffsetX, float cameraOffsetY, float zoomFactor);
void RenderEntities(float cameraOffsetX, float cameraOffsetY, float zoomFactor);
void generateTerrain();

#endif // GAMELOOP_H