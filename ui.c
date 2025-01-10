#include "ui.h"
#include "gameloop.h"
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <math.h>

static GLuint inventorySlotTexture;
static float expGainFlashIntensity = 0.0f;
static Uint32 lastExpGainTime = 0;
static float lastKnownExp = 0.0f;
static GLuint uiShaderProgram;

UIState uiState = {0};

#define HOTBAR_SLOT_SIZE 50.0f
#define HOTBAR_PADDING 5.0f
#define INVENTORY_SLOTS_X 8
#define INVENTORY_SLOTS_Y 4

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
    
    // Initialize exp bar
    glGenVertexArrays(1, &uiState.expBar.vao);
    glGenBuffers(1, &uiState.expBar.vbo);
    
    const float expBarVertices[] = {
        // Positions           // TexCoords
        0.70f,  0.80f,        0.0f, 0.0f,
        0.95f,  0.80f,        1.0f, 0.0f,
        0.95f,  0.85f,        1.0f, 1.0f,
        0.70f,  0.85f,        0.0f, 1.0f
    };

    glBindVertexArray(uiState.expBar.vao);
    glBindBuffer(GL_ARRAY_BUFFER, uiState.expBar.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(expBarVertices), expBarVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Initialize inventory with same vertex format as exp bar
    glGenVertexArrays(1, &uiState.inventory.vao);
    glGenBuffers(1, &uiState.inventory.vbo);
    
    glBindVertexArray(uiState.inventory.vao);
    glBindBuffer(GL_ARRAY_BUFFER, uiState.inventory.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, NULL, GL_DYNAMIC_DRAW); // 4 vertices * 4 components

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    inventorySlotTexture = loadTexture("inventory_slot.bmp");
    
    uiState.inventoryOpen = false;
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
    
    GLint fillLoc = glGetUniformLocation(shaderProgram, "fillAmount");
    glUniform1f(fillLoc, fillAmount);
    
    GLint flashLoc = glGetUniformLocation(shaderProgram, "flashIntensity");
    glUniform1f(flashLoc, expGainFlashIntensity);
    
    // Render exp bar first
    glBindVertexArray(uiState.expBar.vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // Render inventory if open
    if (uiState.inventoryOpen) {
        RenderInventoryUI(player, &uiState);
    }

    // Render hotbar last
    RenderHotbar(player, &uiState);

    // Restore GL state
    if (!blendWasEnabled) glDisable(GL_BLEND);
    glBlendFunc(oldBlendSrc, oldBlendDst);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}
void RenderHotbar(const Player* player, const UIState* state) {
    (void)state; // Unused for now
    
    glUseProgram(uiShaderProgram);
    glBindVertexArray(uiState.inventory.vao);
    
    float totalWidth = INVENTORY_HOTBAR_SIZE * (HOTBAR_SLOT_SIZE + HOTBAR_PADDING) - HOTBAR_PADDING;
    float startX = (2.0f - totalWidth) * 0.5f;
    float startY = -0.9f;

    for (int i = 0; i < INVENTORY_HOTBAR_SIZE; i++) {
        float x = startX + i * (HOTBAR_SLOT_SIZE + HOTBAR_PADDING);
        
        glBindTexture(GL_TEXTURE_2D, inventorySlotTexture);
        
        float vertices[] = {
            x, startY,                         0.0f, 0.0f,
            x + HOTBAR_SLOT_SIZE, startY,      1.0f, 0.0f,
            x + HOTBAR_SLOT_SIZE, startY + HOTBAR_SLOT_SIZE, 1.0f, 1.0f,
            x, startY + HOTBAR_SLOT_SIZE,      0.0f, 1.0f
        };
        
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        Item* item = player->inventory->slots[i];
        if (item) {
            RenderItem(item, x, startY, HOTBAR_SLOT_SIZE);
        }

        if (i == player->inventory->selectedSlot) {
            RenderSlotHighlight(x, startY, HOTBAR_SLOT_SIZE);
        }
    }
}

void RenderInventoryUI(const Player* player, const UIState* state) {
    if (!player || !state) return;
    
    glBindVertexArray(uiState.inventory.vao);
    glBindBuffer(GL_ARRAY_BUFFER, uiState.inventory.vbo);

    // Define panel dimensions
    float panelLeft = 0.5f;
    float panelRight = 0.9f;
    float panelTop = 0.4f;
    float panelBottom = -0.4f;
    
    // Calculate panel dimensions
    float panelWidth = panelRight - panelLeft;
    float panelHeight = panelTop - panelBottom;

    // Draw the panel background
    float vertices[] = {
        // Positions     // TexCoords
        panelLeft,  panelTop,     0.0f, 0.0f,   
        panelRight, panelTop,     1.0f, 0.0f,   
        panelRight, panelBottom,  1.0f, 1.0f,   
        panelLeft,  panelBottom,  0.0f, 1.0f    
    };
        
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glUniform4f(glGetUniformLocation(uiShaderProgram, "color"), 0.2f, 0.2f, 0.2f, 0.9f);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // Calculate slot dimensions based on panel size
    float margin = panelWidth * 0.1f;  // 10% margin
    float usableWidth = panelWidth - (2 * margin);
    float usableHeight = panelHeight - (2 * margin);
    
    float slotSize = usableWidth / INVENTORY_SLOTS_Y;  // Using Y for horizontal slots
    float padding = slotSize * 0.1f;  // 10% of slot size for padding
    slotSize -= padding;  // Adjust slot size to account for padding

    // Calculate starting position to center the grid
    float slotStartX = panelLeft + margin;
    float slotStartY = panelTop - margin;
    
    // Draw slots
    for (int x = 0; x < INVENTORY_SLOTS_Y; x++) {
        for (int y = 0; y < INVENTORY_SLOTS_X; y++) {
            float posX = slotStartX + x * (slotSize + padding);
            float posY = slotStartY - y * (slotSize + padding);

            float slotVertices[] = {
                posX,            posY,            0.0f, 0.0f,
                posX + slotSize, posY,            1.0f, 0.0f,
                posX + slotSize, posY - slotSize, 1.0f, 1.0f,
                posX,            posY - slotSize, 0.0f, 1.0f
            };
            
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(slotVertices), slotVertices);
            glUniform4f(glGetUniformLocation(uiShaderProgram, "color"), 0.3f, 0.3f, 0.3f, 1.0f);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
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