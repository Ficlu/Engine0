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
#define MAX_ENEMIES 20
#define MOVE_SPEED 0.005f
#define GAME_LOGIC_INTERVAL_MS 600
#define FRAME_TIME_MS 24
#define CAMERA_ZOOM 4.00f  
#define TILE_SIZE (2.0f / GRID_SIZE)

// Lerp function declaration
float lerp(float a, float b, float t);

void setGridSize(int size);
void GameLoop();
void Initialize();
void HandleInput();
void UpdateGameLogic();
void Render();
int PhysicsLoop(void* arg);
void CleanUp();
void generateTerrain();

#endif // GAMELOOP_H
