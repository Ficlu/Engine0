#ifndef UI_H
#define UI_H

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include "player.h"

#define EXP_PER_LEVEL 100.0f
#define FLASH_DURATION_MS 500

void InitializeUI(GLuint shaderProgram);
void RenderUI(const Player* player, GLuint shaderProgram);
void CleanupUI(void);

#endif