#ifndef GAMELOOP_H
#define GAMELOOP_H

#include <stdbool.h>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include "grid.h"
#include <stdatomic.h>
#include "pathfinding.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define GRID_SIZE 20
#define TILE_SIZE (2.0f / GRID_SIZE)
#define MAX_ENEMIES 3
#define MOVE_SPEED 0.005f
#define GAME_LOGIC_INTERVAL_MS 24
#define FRAME_TIME_MS 24

// Remove the Tile structure definition from here

void GameLoop();
void Initialize();
void HandleInput();
void UpdateGameLogic();
void Render();
int PhysicsLoop(void* arg);
void CleanUp();

#endif // GAMELOOP_H