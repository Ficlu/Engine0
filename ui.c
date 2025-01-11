#include "ui.h"
#include "gameloop.h"
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <math.h>
#include "rendering.h"

static GLuint inventorySlotTexture;
static float expGainFlashIntensity = 0.0f;
static Uint32 lastExpGainTime = 0;
static float lastKnownExp = 0.0f;
static GLuint uiShaderProgram;

UIState uiState = {0};

#define HOTBAR_SLOT_SIZE 5.0f
#define HOTBAR_PADDING 5.0f
#define INVENTORY_SLOTS_X 6
#define INVENTORY_SLOTS_Y 5

// Forward declarations for static functions
static void RenderItem(const Item* item, float x, float y, float size);
static void RenderSlotHighlight(float x, float y, float size);
static GLuint GetItemTexture(ItemType type);

static GLuint loadTexture(const char* filename) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    // For now, just create a simple checkerboard texture
    unsigned char pixels[] = {
        255, 255, 255,  128, 128, 128,
        128, 128, 128,  255, 255, 255
    };
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    return texture;
}

static GLuint GetItemTexture(ItemType type) {
    (void)type; // Unused for now
    static GLuint defaultTexture = 0;
    if (!defaultTexture) {
        defaultTexture = loadTexture("default_item.bmp");
    }
    return defaultTexture;
}

void InitializeUI(GLuint shaderProgram) {
    uiShaderProgram = shaderProgram;
    
    // Initialize exp bar with sidebar coordinates
    glGenVertexArrays(1, &uiState.expBar.vao);
    glGenBuffers(1, &uiState.expBar.vbo);
    
    // Position the exp bar at the top of the sidebar
    // Using normalized device coordinates for the sidebar viewport
    const float expBarVertices[] = {
        -0.8f,  0.8f,         0.0f, 0.0f,  // Bottom left
         0.8f,  0.8f,         1.0f, 0.0f,  // Bottom right
         0.8f,  0.9f,         1.0f, 1.0f,  // Top right
        -0.8f,  0.9f,         0.0f, 1.0f   // Top left
    };

    glBindVertexArray(uiState.expBar.vao);
    glBindBuffer(GL_ARRAY_BUFFER, uiState.expBar.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(expBarVertices), expBarVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Initialize inventory with coordinates for sidebar
    glGenVertexArrays(1, &uiState.inventory.vao);
    glGenBuffers(1, &uiState.inventory.vbo);
    
    glBindVertexArray(uiState.inventory.vao);
    glBindBuffer(GL_ARRAY_BUFFER, uiState.inventory.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, NULL, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    inventorySlotTexture = loadTexture("inventory_slot.bmp");
    
    uiState.inventoryOpen = true;
    uiState.hoveredSlot = -1;
    uiState.draggedSlot = -1;
    uiState.isDragging = false;
}
void RenderUI(const Player* player, GLuint shaderProgram) {
    glUseProgram(shaderProgram);

    // Save GL state
    GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    GLint oldBlendSrc, oldBlendDst;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &oldBlendSrc);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &oldBlendDst);

    // Setup for UI rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Switch to sidebar viewport
    applyViewport(&sidebarViewport);

    // Calculate exp bar values
    float totalExp = player->skills.experience[SKILL_CONSTRUCTION];
    
    if (totalExp > lastKnownExp) {
        expGainFlashIntensity = 1.0f;
        lastExpGainTime = SDL_GetTicks();
        lastKnownExp = totalExp;
    }
    
    Uint32 currentTime = SDL_GetTicks();
    if (expGainFlashIntensity > 0.0f) {
        Uint32 timeSinceGain = currentTime - lastExpGainTime;
        if (timeSinceGain < FLASH_DURATION_MS) {
            expGainFlashIntensity = 1.0f - ((float)timeSinceGain / FLASH_DURATION_MS);
        } else {
            expGainFlashIntensity = 0.0f;
        }
    }

    float expProgress = fmodf(totalExp, EXP_PER_LEVEL);
    float fillAmount = expProgress / EXP_PER_LEVEL;
    
    // First render exp bar with its special uniforms
    GLint fillLoc = glGetUniformLocation(shaderProgram, "fillAmount");
    GLint flashLoc = glGetUniformLocation(shaderProgram, "flashIntensity");
    
    // Set exp bar specific uniforms
    glUniform1f(fillLoc, fillAmount);
    glUniform1f(flashLoc, expGainFlashIntensity);
    
    // Render exp bar
    glBindVertexArray(uiState.expBar.vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // Reset uniforms for other UI elements
    glUniform1f(fillLoc, 0.0f);
    glUniform1f(flashLoc, 0.0f);

    // Render inventory below exp bar if open
    if (uiState.inventoryOpen) {
        RenderInventoryUI(player, &uiState);
    }

    // Render hotbar at bottom of sidebar
    //RenderHotbar(player, &uiState);

    // Restore GL state
    if (!blendWasEnabled) glDisable(GL_BLEND);
    glBlendFunc(oldBlendSrc, oldBlendDst);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void RenderHotbar(const Player* player, const UIState* state) {
    (void)state;
    
    glUseProgram(uiShaderProgram);
    glBindVertexArray(uiState.inventory.vao);
    
    // Position hotbar at bottom of sidebar
    float totalWidth = 1.6f;  // In normalized device coordinates
    float slotSize = totalWidth / INVENTORY_HOTBAR_SIZE;
    float padding = slotSize * 0.1f;
    float startX = -0.8f;  // Center in sidebar
    float startY = -0.9f;  // Near bottom of sidebar

    for (int i = 0; i < INVENTORY_HOTBAR_SIZE; i++) {
        float x = startX + i * (slotSize + padding);
        
        glBindTexture(GL_TEXTURE_2D, inventorySlotTexture);
        
        float vertices[] = {
            x, startY,                    0.0f, 0.0f,
            x + slotSize, startY,         1.0f, 0.0f,
            x + slotSize, startY + slotSize, 1.0f, 1.0f,
            x, startY + slotSize,         0.0f, 1.0f
        };
        
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        if (player->inventory->slots[i]) {
            RenderItem(player->inventory->slots[i], x, startY, slotSize);
        }

        if (i == player->inventory->selectedSlot) {
            RenderSlotHighlight(x, startY, slotSize);
        }
    }
}
void RenderInventoryUI(const Player* player, const UIState* state) {
    if (!player || !state) return;

    glBindVertexArray(uiState.inventory.vao);
    glBindBuffer(GL_ARRAY_BUFFER, uiState.inventory.vbo);

    // Define panel dimensions
    float sidePadding = 0.0f; // Space between the panel and screen edge
    float panelLeft = -1.0f;
    float panelRight = 1.0f;
    float panelTop = 0.7f;
    float panelBottom = -0.7f;

    // --- Calculate Actual Border Thickness ---
    float borderThicknessNormalized = 0.1f; // From the fragment shader
    float panelWidth = panelRight - panelLeft;
    float panelHeight = panelTop - panelBottom;

    // Convert border thickness from texture space to screen space
    float borderThicknessX = borderThicknessNormalized * panelWidth;
    float borderThicknessY = borderThicknessNormalized * panelHeight;

    // Use the larger of the two to ensure consistency
    float borderThickness = fmaxf(borderThicknessX, borderThicknessY);
    
    // Adjust panel dimensions to account for borders
    float usablePanelLeft = panelLeft;
    float usablePanelRight = panelRight;
    float usablePanelTop = panelTop - borderThickness;
    float usablePanelBottom = panelBottom + borderThickness;

    // Calculate usable panel width and height
    float usablePanelWidth = usablePanelRight - usablePanelLeft;
    float usablePanelHeight = usablePanelTop - usablePanelBottom;

    // Define internal padding (space between grid and panel edges)
    float internalPadding = 0.0f * fminf(usablePanelWidth, usablePanelHeight);

    // Calculate usable grid dimensions (after padding)
    float usableWidth = usablePanelWidth - 2.0f * internalPadding;
    float usableHeight = usablePanelHeight - 2.0f * internalPadding;

    // Determine slot size and enforce square slots
    int rows = INVENTORY_SLOTS_X; // Number of rows
    int cols = INVENTORY_SLOTS_Y; // Number of columns
    float slotSize = fminf(usableWidth / cols, usableHeight / rows); // Maximum square slot size

    // Adjust the usable grid area to reflect the actual grid size with square slots
    float gridWidth = cols * slotSize;
    float gridHeight = rows * slotSize;

    // Center the grid within the usable panel area
    float startX = usablePanelLeft + (usableWidth - gridWidth) / 2.0f;
    float startY = usablePanelTop - internalPadding - (usableHeight - gridHeight) / 2.0f;

    // Render panel background
    float vertices[] = {
        panelLeft,  panelTop,     0.0f, 0.0f,
        panelRight, panelTop,     1.0f, 0.0f,
        panelRight, panelBottom,  1.0f, 1.0f,
        panelLeft,  panelBottom,  0.0f, 1.0f
    };

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glUniform4f(glGetUniformLocation(uiShaderProgram, "color"), 0.3f, 0.3f, 0.3f, 0.9f); // Panel color
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // Render slots
    for (int y = 0; y < rows; y++) {
        float rowY = startY - y * slotSize; // Calculate Y position for the row
        for (int x = 0; x < cols; x++) {
            float posX = startX + x * slotSize; // Calculate X position for the column

            // Draw slot background
            float slotVertices[] = {
                posX,            rowY,            0.0f, 0.0f,
                posX + slotSize, rowY,            1.0f, 0.0f,
                posX + slotSize, rowY - slotSize, 1.0f, 1.0f,
                posX,            rowY - slotSize, 0.0f, 1.0f
            };

            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(slotVertices), slotVertices);
            glUniform4f(glGetUniformLocation(uiShaderProgram, "color"), 0.4f, 0.4f, 0.4f, 1.0f);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

            // Draw slot border
            glLineWidth(1.0f);
            glUniform4f(glGetUniformLocation(uiShaderProgram, "color"), 0.5f, 0.5f, 0.5f, 1.0f);
            glDrawArrays(GL_LINE_LOOP, 0, 4);

            // Render item if present
            int slotIndex = y * cols + x;
            if (player->inventory->slots[slotIndex]) {
                RenderItem(player->inventory->slots[slotIndex], posX, rowY - slotSize, slotSize);
            }
        }
    }
}

static void RenderItem(const Item* item, float x, float y, float size) {
    if (!item) return;

    float vertices[] = {
        // Positions           // TexCoords
        x,          y,         0.0f, 0.0f,
        x + size,   y,         1.0f, 0.0f,
        x + size,   y + size,  1.0f, 1.0f,
        x,          y + size,  0.0f, 1.0f
    };
    
    glUniform4f(glGetUniformLocation(uiShaderProgram, "color"), 0.8f, 0.3f, 0.3f, 1.0f);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}
static void RenderSlotHighlight(float x, float y, float size) {
    // Draw slot background first
    glUniform4f(glGetUniformLocation(uiShaderProgram, "color"), 0.2f, 0.2f, 0.2f, 1.0f);
    
    float vertices[] = {
        x, y,
        x + size, y,
        x + size, y + size,
        x, y + size
    };
    
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // Draw border
    glUniform4f(glGetUniformLocation(uiShaderProgram, "color"), 1.0f, 1.0f, 0.0f, 1.0f);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
}

int GetSlotFromMousePos(int mouseX, int mouseY) {
    (void)mouseX;
    (void)mouseY;
    // Convert mouse position to UI coordinates and determine which slot was clicked
    // This is a placeholder - you'll need to implement the actual conversion
    return -1;
}

void HandleInventoryClick(Player* player, UIState* state, int mouseX, int mouseY) {
    int slot = GetSlotFromMousePos(mouseX, mouseY);
    
    if (slot >= 0 && slot < INVENTORY_SIZE) {
        if (!state->isDragging) {
            if (player->inventory->slots[slot]) {
                state->draggedSlot = slot;
                state->isDragging = true;
            }
        } else {
            SwapSlots(player->inventory, state->draggedSlot, slot);
            state->draggedSlot = -1;
            state->isDragging = false;
        }
    }
}

void CleanupUI(void) {
    glDeleteVertexArrays(1, &uiState.expBar.vao);
    glDeleteBuffers(1, &uiState.expBar.vbo);
    glDeleteVertexArrays(1, &uiState.inventory.vao);
    glDeleteBuffers(1, &uiState.inventory.vbo);
    glDeleteTextures(1, &inventorySlotTexture);
}