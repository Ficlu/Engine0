#ifndef UI_H
#define UI_H

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
#include "player.h"

#define EXP_PER_LEVEL 100.0f
#define FLASH_DURATION_MS 500

// ---- Original Compatibility Structures ----
typedef struct {
    struct {
        GLuint shaderProgram;
        int viewportWidth;
        int viewportHeight;
    } layout;
    void* expBar;
    void* inventory;
    bool inventoryOpen;
    int hoveredSlot;
    int draggedSlot;
    bool isDragging;
} UIState;

extern UIState uiState;

// ---- Core Math Types ----
typedef struct {
    float x, y;
} Vec2f;

typedef struct {
    float m[4][4];
} Mat4f;

typedef struct {
    float r, g, b, a;
} Vec4f;

// ---- Layout System ----
typedef enum {
    UNIT_PIXELS,
    UNIT_PERCENTAGE,
    UNIT_ASPECT
} DimensionUnit;

typedef struct {
    float value;
    DimensionUnit unit;
} Dimension;

typedef struct {
    float min, max;
    bool hasMin, hasMax;
} DimensionConstraint;

// ---- Transform Stack ----
#define MAX_TRANSFORM_STACK 32

typedef struct {
    Mat4f stack[MAX_TRANSFORM_STACK];
    int count;
} TransformStack;

// ---- Layout Context ----
typedef struct {
    float viewportWidth;
    float viewportHeight;
    TransformStack transforms;
    Vec2f currentOrigin;
    Vec2f layoutScale;
} LayoutContext;

// ---- Element Types ----
typedef enum {
    ELEMENT_CONTAINER,
    ELEMENT_GRID,
    ELEMENT_BAR,
    ELEMENT_BUTTON
} ElementType;

// ---- Layout Policies ----
typedef enum {
    LAYOUT_VERTICAL,
    LAYOUT_HORIZONTAL,
    LAYOUT_GRID,
    LAYOUT_ABSOLUTE
} LayoutPolicy;

// ---- Core Element Structure ----
typedef struct UIElement {
    ElementType type;
    LayoutPolicy layout;
    
    // Geometry
    Vec2f position;
    Dimension width, height;
    DimensionConstraint widthConstraint;
    DimensionConstraint heightConstraint;
    float padding[4];
    
    // Transform & bounds
    Mat4f localTransform;
    Mat4f worldTransform;
    Vec2f computedPosition;
    Vec2f computedSize;
    
    // Tree structure
    struct UIElement* parent;
    struct UIElement** children;
    int childCount;
    int childCapacity;
    
    // Rendering
    GLuint vao, vbo;
    Vec4f backgroundColor;
    Vec4f borderColor;
    float borderWidth;
    GLuint textureId;
    bool hasTexture;
    
    // Type-specific data
    union {
        struct {
            int rows;
            int cols;
            float cellAspectRatio;
        } grid;
        
        struct {
            float value;
            float maxValue;
            float flashIntensity;
            uint32_t lastFlashTime;
        } bar;

        struct {
            const Item* item;        // Current item in slot
            float highlightIntensity;  // For hover/selection effects
            bool isSelected;          // Track selection state
            Vec2f itemUV;            // Texture coordinates for item sprite
        } slot;                      // Add this new struct
    } specific;
    
    // Events
    void (*onClick)(struct UIElement* element, int x, int y);
    void (*onHover)(struct UIElement* element, int x, int y);
} UIElement;

// ---- UI Context ----
typedef struct {
    UIElement* root;
    GLuint shaderProgram;
    LayoutContext layoutCtx;
    
    // UI State
    UIElement* hoveredElement;
    UIElement* activeElement;
    bool isDragging;
    
    // Cached transforms
    Mat4f projectionMatrix;
    Mat4f viewportMatrix;
    
    // Previous GL state
    GLint previousViewport[4];
    GLboolean previousBlend;
    GLuint previousProgram;  // Add this line
} UIContext;


// ---- Core Functions ----

// Context management
UIContext* UI_CreateContext(int viewportWidth, int viewportHeight, GLuint shader);
void UI_DestroyContext(UIContext* ctx);

// Element management
UIElement* UI_CreateElement(ElementType type);
void UI_DestroyElement(UIElement* element);
void UI_AddChild(UIElement* parent, UIElement* child);

// Layout system
void UI_BeginLayout(UIContext* ctx);
void UI_EndLayout(UIContext* ctx);
void UI_LayoutElement(UIContext* ctx, UIElement* element);

// Transform management
void UI_PushTransform(UIContext* ctx, const Mat4f* transform);
void UI_PopTransform(UIContext* ctx);
void UI_ComputeWorldTransform(UIContext* ctx, UIElement* element);

// Rendering
void UI_BeginRender(UIContext* ctx);
void UI_EndRender(UIContext* ctx);
void UI_RenderElement(UIContext* ctx, UIElement* element);

// Specialized element creators
UIElement* UI_CreateGrid(int rows, int cols);
UIElement* UI_CreateProgressBar(void);
UIElement* UI_CreateInventorySlot(void);
void UpdateElementVertices(UIElement* element);

// Input handling
void UI_HandleMouseMove(UIContext* ctx, int x, int y);
void UI_HandleMouseClick(UIContext* ctx, int x, int y, bool isDown);

// Utility functions
bool UI_IsPointInElement(const UIElement* element, float x, float y);
void UI_ScreenToNDC(const UIContext* ctx, int screenX, int screenY, float* ndcX, float* ndcY);
void UI_NDCToWorld(const UIContext* ctx, float ndcX, float ndcY, float* worldX, float* worldY);

// Required interface functions for existing codebase
void InitializeUI(GLuint shaderProgram);
void RenderUI(const Player* player, GLuint shaderProgram);
void CleanupUI(void);
void UI_UpdateInventoryGrid(UIElement* grid, const Inventory* inv);
void UI_RenderInventorySlot(UIContext* ctx, UIElement* slot, const Item* item);
#endif // UI_H