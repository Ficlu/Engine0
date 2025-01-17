// input.h
#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>
#include "structures.h"

// Coordinate conversion functions
typedef struct {
    int gridX;
    int gridY;
} GridCoordinates;

typedef struct {
    float worldX;
    float worldY;
} WorldCoordinates;

GridCoordinates WindowToGridCoordinates(int mouseX, int mouseY, float cameraX, float cameraY, float zoomFactor);
bool IsWithinPlayerRange(int gridX, int gridY, int playerX, int playerY);
void HandleInput(void);
static void HandleMouseInput(SDL_Event* event);
static void HandleZoom(int wheelDelta);
#endif // INPUT_H