#include "ui.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "gameloop.h"
#include "player.h"
#include "rendering.h"

// Global state
UIState uiState = {0};
static UIContext* gContext = NULL;

// ---- Math Utilities ----

static Mat4f Mat4f_Identity(void) {
    Mat4f m = {{
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    }};
    return m;
}

static Mat4f Mat4f_Translate(float x, float y) {
    Mat4f m = Mat4f_Identity();
    m.m[3][0] = x;
    m.m[3][1] = y;
    return m;
}

static Mat4f Mat4f_Scale(float x, float y) {
    Mat4f m = Mat4f_Identity();
    m.m[0][0] = x;
    m.m[1][1] = y;
    return m;
}

static Mat4f Mat4f_Multiply(const Mat4f* a, const Mat4f* b) {
    Mat4f result = {0};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                result.m[i][j] += a->m[i][k] * b->m[k][j];
            }
        }
    }
    return result;
}

static void Vec2f_Transform(Vec2f* result, const Vec2f* v, const Mat4f* m) {
    float x = v->x * m->m[0][0] + v->y * m->m[1][0] + m->m[3][0];
    float y = v->x * m->m[0][1] + v->y * m->m[1][1] + m->m[3][1];
    result->x = x;
    result->y = y;
}

// ---- Memory Management ----

static void* UI_Malloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "UI: Failed to allocate %zu bytes\n", size);
        exit(1);
    }
    return ptr;
}
void UI_CleanupInventoryGrid(UIElement* grid) {
    if (!grid) return;
    for (int i = 0; i < grid->childCount; i++) {
        UIElement* slot = grid->children[i];
        if (slot) {
            slot->specific.slot.item = NULL; // Just clear the reference
        }
    }
}
static void* UI_Calloc(size_t count, size_t size) {
    void* ptr = calloc(count, size);
    if (!ptr) {
        fprintf(stderr, "UI: Failed to allocate %zu bytes\n", count * size);
        exit(1);
    }
    return ptr;
}

// ---- Transform Stack Management ----

static void TransformStack_Init(TransformStack* stack) {
    stack->count = 0;
    stack->stack[0] = Mat4f_Identity();
    stack->count = 1;
}

static void TransformStack_Push(TransformStack* stack, const Mat4f* transform) {
    if (stack->count >= MAX_TRANSFORM_STACK) {
        fprintf(stderr, "UI: Transform stack overflow\n");
        return;
    }
    
    stack->stack[stack->count] = Mat4f_Multiply(&stack->stack[stack->count - 1], transform);
    stack->count++;
}

static void TransformStack_Pop(TransformStack* stack) {
    if (stack->count <= 1) {
        fprintf(stderr, "UI: Transform stack underflow\n");
        return;
    }
    stack->count--;
}

static const Mat4f* TransformStack_Current(const TransformStack* stack) {
    return &stack->stack[stack->count - 1];
}

// ---- Vertex Buffer Management ----

static void InitializeElementBuffers(UIElement* element) {
    glGenVertexArrays(1, &element->vao);
    glGenBuffers(1, &element->vbo);
    
    glBindVertexArray(element->vao);
    glBindBuffer(GL_ARRAY_BUFFER, element->vbo);
    
    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texcoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// ---- Element Creation and Management ----

UIContext* UI_CreateContext(int viewportWidth, int viewportHeight, GLuint shader) {
    printf("Creating UI context: viewport=%dx%d, shader=%d\n", 
           viewportWidth, viewportHeight, shader);
    
    UIContext* ctx = UI_Calloc(1, sizeof(UIContext));
    
    ctx->shaderProgram = shader;
    ctx->layoutCtx.viewportWidth = (float)SIDEBAR_WIDTH;  // Use SIDEBAR_WIDTH instead
    ctx->layoutCtx.viewportHeight = (float)WINDOW_HEIGHT;
    
    TransformStack_Init(&ctx->layoutCtx.transforms);
    
    // Create root element
    ctx->root = UI_CreateElement(ELEMENT_CONTAINER);
    
    // Set root element to sidebar dimensions
    ctx->root->computedPosition = (Vec2f){0, 0};
    ctx->root->computedSize = (Vec2f){SIDEBAR_WIDTH, WINDOW_HEIGHT};
    ctx->root->layout = LAYOUT_VERTICAL;
    ctx->root->backgroundColor = (Vec4f){0.2f, 0.2f, 0.2f, 1.0f};
    
    printf("Root element size: %.2f x %.2f\n", 
           ctx->root->computedSize.x, ctx->root->computedSize.y);
    
    return ctx;
}
UIElement* UI_CreateElement(ElementType type) {
    UIElement* element = UI_Calloc(1, sizeof(UIElement));
    
    element->type = type;
    element->localTransform = Mat4f_Identity();
    element->worldTransform = Mat4f_Identity();
    
    element->width.unit = UNIT_PIXELS;
    element->height.unit = UNIT_PIXELS;
    
    element->childCapacity = 4;
    element->children = UI_Malloc(element->childCapacity * sizeof(UIElement*));
    
    element->backgroundColor = (Vec4f){0.2f, 0.2f, 0.2f, 1.0f};
    element->borderColor = (Vec4f){0.8f, 0.8f, 0.8f, 1.0f};
    element->borderWidth = 1.0f;
    
    InitializeElementBuffers(element);
    
    return element;
}

void UI_DestroyElement(UIElement* element) {
    if (!element) return;
    
    for (int i = 0; i < element->childCount; i++) {
        UI_DestroyElement(element->children[i]);
    }
    
    if (element->children) {
        free(element->children);
    }
    
    if (element->vao) {
        glDeleteVertexArrays(1, &element->vao);
    }
    if (element->vbo) {
        glDeleteBuffers(1, &element->vbo);
    }
    
    free(element);
}

void UI_DestroyContext(UIContext* ctx) {
    if (!ctx) return;
    
    if (ctx->root) {
        UI_DestroyElement(ctx->root);
    }
    
    free(ctx);
}

void UI_AddChild(UIElement* parent, UIElement* child) {
    if (!parent || !child) return;
    
    if (parent->childCount >= parent->childCapacity) {
        size_t newCapacity = parent->childCapacity * 2;
        UIElement** newChildren = realloc(parent->children, 
                                        newCapacity * sizeof(UIElement*));
        if (!newChildren) return;
        
        parent->children = newChildren;
        parent->childCapacity = newCapacity;
    }
    
    child->parent = parent;
    parent->children[parent->childCount++] = child;
}

// ---- Layout Calculations ----

static void CalculateElementDimensions(UIElement* element, float parentWidth, float parentHeight) {
    // Calculate width
    if (element->width.unit == UNIT_PERCENTAGE) {
        element->computedSize.x = parentWidth * (element->width.value / 100.0f);
    } else if (element->width.unit == UNIT_PIXELS) {
        element->computedSize.x = element->width.value;
    }
    
    // Calculate height
    if (element->height.unit == UNIT_PERCENTAGE) {
        element->computedSize.y = parentHeight * (element->height.value / 100.0f);
    } else if (element->height.unit == UNIT_PIXELS) {
        element->computedSize.y = element->height.value;
    } else if (element->height.unit == UNIT_ASPECT) {
        // Height is determined by width and aspect ratio
        element->computedSize.y = element->computedSize.x * element->height.value;
    }
    
    // Apply constraints if they exist
    if (element->widthConstraint.hasMin) {
        element->computedSize.x = fmaxf(element->computedSize.x, element->widthConstraint.min);
    }
    if (element->widthConstraint.hasMax) {
        element->computedSize.x = fminf(element->computedSize.x, element->widthConstraint.max);
    }
    
    if (element->heightConstraint.hasMin) {
        element->computedSize.y = fmaxf(element->computedSize.y, element->heightConstraint.min);
    }
    if (element->heightConstraint.hasMax) {
        element->computedSize.y = fminf(element->computedSize.y, element->heightConstraint.max);
    }
}

// ---- Layout System ----

static void LayoutVerticalContainer(UIContext* ctx, UIElement* container) {
    float yOffset = container->padding[0]; // Start after top padding
    
    for (int i = 0; i < container->childCount; i++) {
        UIElement* child = container->children[i];
        
        float childX = container->padding[3]; // Left padding
        float childY = yOffset;
        float childWidth = container->computedSize.x - (container->padding[1] + container->padding[3]);
        float availableHeight = container->computedSize.y - yOffset - container->padding[2];
        
        CalculateElementDimensions(child, childWidth, availableHeight);
        
        child->computedPosition.x = childX;
        child->computedPosition.y = childY;
        
        UI_LayoutElement(ctx, child);
        
        yOffset += child->computedSize.y + container->padding[0]; // Add spacing between elements
    }
}

static void LayoutGridContainer(UIContext* ctx, UIElement* container) {
    float cellWidth = (container->computedSize.x - (container->padding[1] + container->padding[3])) / 
                      container->specific.grid.cols;
    
    // For grids, maintain square cells based on width
    float cellHeight = cellWidth;
    
    float startX = container->padding[3];
    float startY = container->padding[0];
    
    for (int row = 0; row < container->specific.grid.rows; row++) {
        for (int col = 0; col < container->specific.grid.cols; col++) {
            int index = row * container->specific.grid.cols + col;
            if (index >= container->childCount) break;
            
            UIElement* child = container->children[index];
            
            child->computedPosition.x = startX + (col * cellWidth);
            child->computedPosition.y = startY + (row * cellHeight);
            
            CalculateElementDimensions(child, cellWidth, cellHeight);
            
            UI_LayoutElement(ctx, child);
        }
    }
}

void UI_LayoutElement(UIContext* ctx, UIElement* element) {
    if (!element) return;
    
    // Calculate local transform based on computed position
    Mat4f localTransform = Mat4f_Translate(element->computedPosition.x, element->computedPosition.y);
    element->localTransform = localTransform;
    
    // Update world transform
    if (element->parent) {
        element->worldTransform = Mat4f_Multiply(&element->parent->worldTransform, &element->localTransform);
    } else {
        element->worldTransform = element->localTransform;
    }
    
    // Layout children based on container type
    switch (element->layout) {
        case LAYOUT_VERTICAL:
            LayoutVerticalContainer(ctx, element);
            break;
        case LAYOUT_GRID:
            LayoutGridContainer(ctx, element);
            break;
        default:
            // Handle other layout types as needed
            break;
    }
}

void UI_BeginLayout(UIContext* ctx) {
    if (!ctx || !ctx->root) return;
    
    // Calculate layout for each child
    float yOffset = 10.0f;  // Start with top margin
    
    for (int i = 0; i < ctx->root->childCount; i++) {
        UIElement* child = ctx->root->children[i];
        if (!child) continue;
        
        // Set position and size based on element type
        if (child->type == ELEMENT_BAR) {
            child->computedPosition.x = 10.0f;
            child->computedPosition.y = yOffset;
            child->computedSize.x = SIDEBAR_WIDTH - 20.0f;
            child->computedSize.y = 30.0f;
            yOffset += 40.0f;  // Bar height + spacing
        }
        else if (child->type == ELEMENT_GRID) {
            child->computedPosition.x = 10.0f;
            child->computedPosition.y = yOffset;
            child->computedSize.x = SIDEBAR_WIDTH - 20.0f;
            child->computedSize.y = WINDOW_HEIGHT - yOffset - 10.0f;  // Remaining space minus bottom margin
        }
        
        printf("Child %d layout: pos=(%.2f, %.2f) size=(%.2f, %.2f)\n",
               i, child->computedPosition.x, child->computedPosition.y,
               child->computedSize.x, child->computedSize.y);
    }
}

// ---- Rendering System ----
// In ui.c add this implementation:

void UpdateElementVertices(UIElement* element) {
    float x1 = element->computedPosition.x;
    float y1 = element->computedPosition.y;
    float x2 = x1 + element->computedSize.x;
    float y2 = y1 + element->computedSize.y;
    
    // Transform to NDC coordinates
    x1 = (2.0f * x1 / SIDEBAR_WIDTH) - 1.0f;
    x2 = (2.0f * x2 / SIDEBAR_WIDTH) - 1.0f;
    y1 = 1.0f - (2.0f * y1 / WINDOW_HEIGHT);
    y2 = 1.0f - (2.0f * y2 / WINDOW_HEIGHT);

    float texX = 0.0f, texY = 0.0f;
    float texWidth = 1.0f/3.0f, texHeight = 1.0f/6.0f;
    
    if (element->type == ELEMENT_CONTAINER && element->specific.slot.item) {
        if (element->specific.slot.item->type == ITEM_FERN) {
            // Fern is in the first row (top), one-third across
            texX = 1.0f/3.0f;  // Start 1/3 across
            texY = 0.0f;       // Start at top
        }
    }

    // Notice y coordinate inversion for proper texture orientation
    float vertices[] = {
        // Positions        // Texture coords
        x1, y1,            texX,              texY + texHeight,  // Top left
        x2, y1,            texX + texWidth,   texY + texHeight,  // Top right
        x1, y2,            texX,              texY,              // Bottom left
        
        x2, y1,            texX + texWidth,   texY + texHeight,  // Top right
        x2, y2,            texX + texWidth,   texY,              // Bottom right
        x1, y2,            texX,              texY               // Bottom left
    };
    glBindBuffer(GL_ARRAY_BUFFER, element->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(element->vao);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void UI_EndRender(UIContext* ctx) {
    if (!ctx) return;
    
    // Restore previous shader program
    glUseProgram(ctx->previousProgram);
    
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    
    // Check for any remaining GL errors
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        printf("GL error at end of UI render: 0x%x\n", err);
    }
}

void UI_BeginRender(UIContext* ctx) {
    if (!ctx || !ctx->shaderProgram) {
        printf("ERROR: Invalid UI context or shader program\n");
        return;
    }
    
    // Store previous GL state
    GLint previous_program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previous_program);
    
    // Switch to UI shader and verify
    glUseProgram(ctx->shaderProgram);
    GLint current_program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
    printf("Shader switch: previous=%d, current=%d, expected=%d\n", 
           previous_program, current_program, ctx->shaderProgram);
    
    // Set up blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Set and verify viewport
    glViewport(GAME_VIEW_WIDTH, 0, SIDEBAR_WIDTH, WINDOW_HEIGHT);
    
    // Clear sidebar area with a distinct color for debugging
    glEnable(GL_SCISSOR_TEST);
    glScissor(GAME_VIEW_WIDTH, 0, SIDEBAR_WIDTH, WINDOW_HEIGHT);
    glClearColor(0.2f, 0.2f, 0.3f, 1.0f);  // Slightly blue tint for visibility
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Verify all uniforms are accessible
    GLint colorLoc = glGetUniformLocation(ctx->shaderProgram, "uColor");
    GLint borderColorLoc = glGetUniformLocation(ctx->shaderProgram, "uBorderColor");
    printf("Shader uniforms: color=%d, border=%d\n", colorLoc, borderColorLoc);
    
    // Store in context for restoration
    ctx->previousProgram = previous_program;
}
// Update initial element positions in UI_CreateProgressBar:
UIElement* UI_CreateProgressBar(void) {
    UIElement* bar = UI_CreateElement(ELEMENT_BAR);
    if (!bar) return NULL;
    
    // Position relative to sidebar
    bar->computedPosition.x = 10.0f;  // 10 pixels margin
    bar->computedPosition.y = 10.0f;  // 10 pixels from top
    bar->computedSize.x = SIDEBAR_WIDTH - 20.0f;  // Full width minus margins
    bar->computedSize.y = 30.0f;  // Fixed height
    
    bar->backgroundColor = (Vec4f){0.2f, 0.7f, 0.2f, 1.0f};  // Green
    bar->borderColor = (Vec4f){0.8f, 0.8f, 0.8f, 1.0f};     // Light gray
    bar->borderWidth = 1.0f;
    
    return bar;
}


// ---- Interface Functions ----

void InitializeUI(GLuint shaderProgram) {
    printf("Initializing UI with shader program: %d\n", shaderProgram);
    
    // Create UI context
    gContext = UI_CreateContext(SIDEBAR_WIDTH, WINDOW_HEIGHT, shaderProgram);
    if (!gContext) {
        printf("ERROR: Failed to create UI context\n");
        return;
    }

    // Create progress bar with bright color for visibility
    UIElement* expBar = UI_CreateProgressBar();
    if (expBar) {
        expBar->backgroundColor = (Vec4f){0.0f, 1.0f, 0.0f, 1.0f};  // Bright green
        expBar->borderColor = (Vec4f){1.0f, 1.0f, 1.0f, 1.0f};      // White border
        expBar->borderWidth = 2.0f;  // Thicker border for visibility
        uiState.expBar = expBar;
        UI_AddChild(gContext->root, expBar);
    }

    // Create inventory grid with visible colors
    UIElement* inventoryGrid = UI_CreateGrid(5, 5);
    if (inventoryGrid) {
        inventoryGrid->backgroundColor = (Vec4f){0.3f, 0.3f, 0.3f, 1.0f};
        inventoryGrid->borderColor = (Vec4f){0.8f, 0.8f, 0.8f, 1.0f};
        inventoryGrid->borderWidth = 1.0f;
        uiState.inventory = inventoryGrid;
        UI_AddChild(gContext->root, inventoryGrid);
    }
}
void RenderUI(const Player* player, GLuint shaderProgram){
    if (!gContext || !player) {
        printf("RenderUI: NULL context or player\n");
        return;
    }

    UI_BeginLayout(gContext);
    
    // Update inventory grid with current items
    if (uiState.inventory) {
        printf("Updating inventory in RenderUI - player inventory has %d items\n", 
               player->inventory->slotCount);
        UI_UpdateInventoryGrid((UIElement*)uiState.inventory, player->inventory);
    } else {
        printf("No inventory UI element found\n");
    }

    UI_BeginRender(gContext);
    
    // Modify UI_RenderElement to handle inventory slots
    UI_RenderElement(gContext, gContext->root);
    
    UI_EndRender(gContext);
}

void CleanupUI(void) {
    if (gContext) {
        if (uiState.inventory) {
            UI_CleanupInventoryGrid((UIElement*)uiState.inventory);
        }
        UI_DestroyContext(gContext);
        gContext = NULL;
    }
    memset(&uiState, 0, sizeof(uiState));
}

// Original working code with the fix for cell position centering
UIElement* UI_CreateGrid(int rows, int cols) {
    UIElement* grid = UI_CreateElement(ELEMENT_GRID);
    if (!grid) return NULL;
    
    // Position relative to sidebar
    grid->computedPosition.x = 10.0f;
    grid->computedPosition.y = 100.0f;  
    
    grid->computedSize.x = SIDEBAR_WIDTH - 20.0f;
    grid->computedSize.y = 400.0f;
    
    grid->layout = LAYOUT_GRID;
    grid->specific.grid.rows = rows;
    grid->specific.grid.cols = cols;
    
    grid->backgroundColor = (Vec4f){0.3f, 0.3f, 0.4f, 1.0f};
    grid->borderColor = (Vec4f){0.6f, 0.6f, 0.6f, 1.0f};
    grid->borderWidth = 2.0f;

    float availableWidth = grid->computedSize.x - (grid->borderWidth * (cols + 1));
    float availableHeight = grid->computedSize.y - (grid->borderWidth * (rows + 1));
    float cellSize = fminf(availableWidth / cols, availableHeight / rows);

    float totalGridWidth = cols * cellSize + (cols + 1) * grid->borderWidth;
    float totalGridHeight = rows * cellSize + (rows + 1) * grid->borderWidth;
    
    // Here's the fix - startX should be relative to the grid's total available width
    float startX = (grid->computedSize.x - totalGridWidth) / 2.0f + grid->computedPosition.x;
    float startY = (grid->computedSize.y - totalGridHeight) / 2.0f;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            UIElement* cell = UI_CreateElement(ELEMENT_CONTAINER);
            if (!cell) continue;

            cell->computedPosition.x = startX + j * (cellSize + grid->borderWidth) + grid->borderWidth;
            cell->computedPosition.y = startY + i * (cellSize + grid->borderWidth) + grid->borderWidth;
            cell->computedSize.x = cellSize;
            cell->computedSize.y = cellSize;

            cell->backgroundColor = (Vec4f){0.2f, 0.2f, 0.3f, 1.0f};
            cell->borderColor = (Vec4f){0.4f, 0.4f, 0.4f, 1.0f};
            cell->borderWidth = 1.0f;

            UI_AddChild(grid, cell);
        }
    }
    
    return grid;
}

void UI_RenderElement(UIContext* ctx, UIElement* element) {
    if (!element) return;
    
    UpdateElementVertices(element);
    
    glUseProgram(ctx->shaderProgram);
    
    GLint colorLoc = glGetUniformLocation(ctx->shaderProgram, "uColor");
    GLint borderColorLoc = glGetUniformLocation(ctx->shaderProgram, "uBorderColor");
    GLint borderWidthLoc = glGetUniformLocation(ctx->shaderProgram, "uBorderWidth");
    GLint hasTextureLoc = glGetUniformLocation(ctx->shaderProgram, "uHasTexture");
    GLint textureLoc = glGetUniformLocation(ctx->shaderProgram, "textureAtlas");
    
    // Debug uniform locations
    printf("Uniform locations: color=%d, border=%d, hasTexture=%d, texture=%d\n",
           colorLoc, borderColorLoc, hasTextureLoc, textureLoc);

    glBindVertexArray(element->vao);
    
    // First render the background
    if (colorLoc >= 0) {
        glUniform4fv(colorLoc, 1, (float*)&element->backgroundColor);
    }
    if (borderColorLoc >= 0) {
        glUniform4fv(borderColorLoc, 1, (float*)&element->borderColor);
    }
    if (borderWidthLoc >= 0) {
        glUniform1f(borderWidthLoc, element->borderWidth);
    }
    if (hasTextureLoc >= 0) {
        glUniform1i(hasTextureLoc, 0);  // Start with no texture
    }
    
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // If this is a slot with an item, render the item texture
    if (element->type == ELEMENT_CONTAINER && element->specific.slot.item) {
        // Enable texturing for item
        if (hasTextureLoc >= 0) {
            glUniform1i(hasTextureLoc, 1);
        }
        if (textureLoc >= 0) {
            glUniform1i(textureLoc, 0);
        }
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureAtlas);
        
        // Debug texture binding
        GLint boundTexture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTexture);
        printf("Rendering item texture: atlas=%u, bound=%d, UV=(%.2f, %.2f)\n",
               textureAtlas, boundTexture,
               element->specific.slot.itemUV.x, element->specific.slot.itemUV.y);

        // Render the item
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // Check for GL errors
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            printf("GL error after item render: 0x%x\n", err);
        }

        // Reset texture state
        if (hasTextureLoc >= 0) {
            glUniform1i(hasTextureLoc, 0);
        }
    }

    // Render children
    for (int i = 0; i < element->childCount; i++) {
        UI_RenderElement(ctx, element->children[i]);
    }
}
UIElement* UI_CreateInventorySlot(void) {
    UIElement* slot = UI_CreateElement(ELEMENT_CONTAINER);
    if (!slot) return NULL;
    
    slot->backgroundColor = (Vec4f){0.2f, 0.2f, 0.2f, 0.9f};
    slot->borderColor = (Vec4f){0.4f, 0.4f, 0.4f, 1.0f};
    slot->borderWidth = 1.0f;
    
    // Initialize slot-specific data
    slot->specific.slot.item = NULL;
    slot->specific.slot.highlightIntensity = 0.0f;
    slot->specific.slot.isSelected = false;
    slot->specific.slot.itemUV = (Vec2f){0.0f, 0.0f};
    
    return slot;
}

void UI_SetSlotItem(UIElement* slot, const Item* item) {
    if (!slot) return;
    
    slot->specific.slot.item = NULL;
    
    if (item) {
        slot->specific.slot.item = item;
        
        // Match the coordinates used in the grid
        switch(item->type) {
            case ITEM_FERN:
                slot->specific.slot.itemUV.x = 1.0f/3.0f;
                slot->specific.slot.itemUV.y = 0.0f/6.0f;
                break;
            default:
                float spriteSize = 1.0f / 8.0f;
                slot->specific.slot.itemUV.x = (item->type % 8) * spriteSize;
                slot->specific.slot.itemUV.y = (item->type / 8) * spriteSize;
                break;
        }
        
        printf("Set slot item: type=%d, UV=(%.2f, %.2f) [VERIFIED]\n", 
               item->type, slot->specific.slot.itemUV.x, slot->specific.slot.itemUV.y);
    }
}

void UI_RenderInventorySlot(UIContext* ctx, UIElement* slot, const Item* item) {
    if (!slot || !ctx->shaderProgram || !item) return;

    // Add debug output
    printf("Rendering slot with item type %d\n", item->type);

    float padding = slot->borderWidth + 2.0f;
    float x1 = slot->computedPosition.x + padding;
    float y1 = slot->computedPosition.y + padding;
    float x2 = x1 + slot->computedSize.x - (padding * 2);
    float y2 = y1 + slot->computedSize.y - (padding * 2);

    x1 = (2.0f * x1 / SIDEBAR_WIDTH) - 1.0f;
    x2 = (2.0f * x2 / SIDEBAR_WIDTH) - 1.0f;
    y1 = 1.0f - (2.0f * y1 / WINDOW_HEIGHT);
    y2 = 1.0f - (2.0f * y2 / WINDOW_HEIGHT);

    // Use the stored UV coordinates
    float texX = slot->specific.slot.itemUV.x;
    float texY = slot->specific.slot.itemUV.y;
    float texWidth = 1.0f/3.0f;
    float texHeight = 1.0f/6.0f;

    printf("Drawing item with tex coords: (%.2f, %.2f) to (%.2f, %.2f)\n", 
           texX, texY, texX + texWidth, texY + texHeight);

    float vertices[] = {
        x1, y1,    texX, texY + texHeight,
        x2, y1,    texX + texWidth, texY + texHeight,
        x2, y2,    texX + texWidth, texY,
        x2, y2,    texX + texWidth, texY,
        x1, y2,    texX, texY,
        x1, y1,    texX, texY + texHeight
    };

    // Verify GL state before render
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    printf("Current shader program: %d (expected: %d)\n", currentProgram, ctx->shaderProgram);

    glBindBuffer(GL_ARRAY_BUFFER, slot->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    GLint hasTextureLoc = glGetUniformLocation(ctx->shaderProgram, "uHasTexture");
    GLint textureLoc = glGetUniformLocation(ctx->shaderProgram, "textureAtlas");
    
    // Add error checking
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        printf("GL error before setting uniforms: 0x%x\n", err);
    }

    if (hasTextureLoc >= 0 && textureLoc >= 0) {
        glUniform1i(hasTextureLoc, 1);
        glUniform1i(textureLoc, 0);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureAtlas);
        
        // Verify texture binding
        GLint boundTexture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTexture);
        printf("Bound texture: %d (expected: %d)\n", boundTexture, textureAtlas);

        glBindVertexArray(slot->vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        err = glGetError();
        if (err != GL_NO_ERROR) {
            printf("GL error after draw: 0x%x\n", err);
        }

        glUniform1i(hasTextureLoc, 0);
    } else {
        printf("ERROR: Shader uniforms not found - hasTexture=%d, texture=%d\n", 
               hasTextureLoc, textureLoc);
    }
}


void UI_UpdateInventoryGrid(UIElement* grid, const Inventory* inv) {
    if (!grid || !inv) {
        printf("UI_UpdateInventoryGrid: NULL grid or inventory\n");
        return;
    }

    printf("Updating inventory grid - grid has %d children, inventory has %d items\n",
           grid->childCount, inv->slotCount);

    for (int i = 0; i < grid->childCount && i < INVENTORY_SIZE; i++) {
        UIElement* slot = grid->children[i];
        if (!slot) continue;

        const Item* item = inv->slots[i];
        if (item) {
            printf("Slot %d: Setting item type %d\n", i, item->type);
        } else {
            printf("Slot %d: Empty\n", i);
        }

        UI_SetSlotItem(slot, item);
    }
}