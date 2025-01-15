#ifndef UI_H
#define UI_H

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
#include "player.h"
#include "item.h" 

#define EXP_PER_LEVEL 800.0f
#define FLASH_DURATION_MS 500
#define MAX_BATCH_VERTICES 30000
#define VERTICES_PER_QUAD 6
#define MAX_GRID_CELLS 256  // Support up to 16x16 grids

// Forward declare Item to break circular dependency
struct Item;

// Core rendering structures
typedef struct {
    float position[2];
    float texCoords[2];
    float color[4];
} BatchVertex;

typedef struct {
    BatchVertex* vertices;
    GLuint vao;
    GLuint vbo;
    int vertexCount;
    bool initialized;
} UIBatchRenderer;

// Core math types
typedef struct { float x, y; } UIVec2f;  // Renamed to avoid conflicts
typedef struct { float r, g, b, a; } UIVec4f;

// Grid cell data
typedef struct {
    UIVec2f position;
    UIVec2f size;
    bool hasItem;
    const Item* item;
    float itemTexX;
    float itemTexY;
} UICellData;

typedef enum {
    ELEMENT_CONTAINER,
    ELEMENT_GRID,
    ELEMENT_BAR,
    ELEMENT_BUTTON
} ElementType;

// Core element structure
typedef struct UIElement {
    ElementType type;
    
    // Geometry
    UIVec2f position;
    UIVec2f size;
    float padding[4];
    
    // Visual properties
    UIVec4f backgroundColor;
    UIVec4f borderColor;
    float borderWidth;
    
    // Type-specific data
    union {
        struct {
            int rows;
            int cols;
            float cellAspectRatio;
            UICellData cells[MAX_GRID_CELLS];
        } grid;
        
        struct {
            float value;
            float maxValue;
            float flashIntensity;
            uint32_t lastFlashTime;
        } bar;

        struct {
            const struct Item* item;
            float highlightIntensity;
            bool isSelected;
            UIVec2f itemUV;
        } slot;
    } specific;
} UIElement;

// UI Context - simplified for batch rendering
typedef struct {
    UIElement* root;
    UIBatchRenderer batchRenderer;
    GLuint shaderProgram;
    
    // Viewport info
    int viewportWidth;
    int viewportHeight;
    
    // State tracking
    UIElement* hoveredElement;
    UIElement* activeElement;
    bool isDragging;
} UIContext;

typedef struct {
    void* expBar;  // Use void* to avoid circular dependency
    void* inventory;
    bool inventoryOpen;
    int hoveredSlot;
    int draggedSlot;
    bool isDragging;
} UIState;

extern UIState uiState;  // Declare it external

// Core batch rendering functions
void UI_InitBatchRenderer(UIBatchRenderer* renderer);
void UI_BeginBatch(UIContext* ctx);
void UI_EndBatch(UIContext* ctx);
void UI_FlushBatch(UIContext* ctx);

// Element management
UIElement* UI_CreateElement(ElementType type);
void UI_DestroyElement(UIElement* element);

// Core rendering interface
void UI_BeginRender(UIContext* ctx);
void UI_EndRender(UIContext* ctx);
void UI_RenderElement(UIContext* ctx, UIElement* element);

// Specialized element creators
UIElement* UI_CreateGrid(int rows, int cols);
UIElement* UI_CreateProgressBar(void);
UIElement* UI_CreateInventorySlot(void);

// Required interface functions for game
void InitializeUI(GLuint shaderProgram);
void RenderUI(const Player* player);  // Removed redundant shader parameter
void CleanupUI(void);
void UI_UpdateInventoryGrid(UIElement* grid, const Inventory* inv);
void UI_RenderSidebarBackground(UIContext* ctx);

#endif // UI_H