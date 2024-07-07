#ifndef GAMELOOP_H
#define GAMELOOP_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <GL/glew.h>
#include "player.h"
#include "enemy.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define GRID_SIZE 20
#define TILE_SIZE (2.0f / GRID_SIZE)  // Size of a tile in normalized device coordinates
#define MOVE_SPEED 0.05f
#define GAME_LOGIC_INTERVAL_MS 600
#define TARGET_FPS 200
#define FRAME_TIME_MS (1000 / TARGET_FPS)
#define MAX_ENEMIES 10

typedef struct {
    bool walkable;
} Tile;

// Function declarations
void GameLoop();
void Initialize();
void HandleInput();
void UpdateGameLogic();
void Render();
void CleanUp();
int PhysicsLoop(void* arg);

extern atomic_bool isRunning;
extern SDL_Window* window;
extern SDL_GLContext mainContext;
extern GLuint shaderProgram;
extern GLuint gridVAO;
extern GLuint squareVAO;
extern GLuint squareVBO;
extern int vertexCount;
extern Tile grid[GRID_SIZE][GRID_SIZE];
extern Player player;
extern Enemy enemies[MAX_ENEMIES];

#endif // GAMELOOP_H
