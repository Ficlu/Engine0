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
#define MAX_ENEMIES 5
#define MOVE_SPEED 0.01f
#define GAME_LOGIC_INTERVAL_MS 16
#define FRAME_TIME_MS 16

// Remove the Tile structure definition from here

void GameLoop();
void Initialize();
void HandleInput();
void UpdateGameLogic();
void Render();
int PhysicsLoop(void* arg);
void CleanUp();

#endif // GAMELOOP_H