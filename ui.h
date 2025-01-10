#ifndef UI_H
#define UI_H

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include "player.h"

#define EXP_PER_LEVEL 100.0f
#define FLASH_DURATION_MS 500

// Add to ui.h:
typedef struct {
    GLuint vao;
    GLuint vbo;
} UIElement;

// Update UIState struct:
typedef struct {
    bool inventoryOpen;
    int hoveredSlot;
    int draggedSlot;
    bool isDragging;
    UIElement expBar;
    UIElement inventory;
} UIState;

extern UIState uiState;  // Add this line


void InitializeUI(GLuint shaderProgram);
void RenderUI(const Player* player, GLuint shaderProgram);
void RenderInventoryUI(const Player* player, const UIState* state);
void RenderHotbar(const Player* player, const UIState* state);
void HandleInventoryClick(Player* player, UIState* state, int mouseX, int mouseY);
void CleanupUI(void);

#endif