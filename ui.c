// ui.c - Core UI Implementation with batch rendering
#include "ui.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gameloop.h"
#include "rendering.h"
#include "texture_coords.h"
static UIContext* gContext = NULL;
UIState uiState = {0};
/**
 * @brief Initializes the batch renderer for UI elements.
 * 
 * @param renderer A pointer to the batch renderer to be initialized.
 * 
 * Allocates memory for the batch renderer's vertex buffer, sets up OpenGL 
 * vertex array objects (VAOs) and vertex buffer objects (VBOs), and configures
 * vertex attributes for position, texture coordinates, and color.
 */
void UI_InitBatchRenderer(UIBatchRenderer* renderer) {
    if (renderer->initialized) return;

    renderer->vertices = (BatchVertex*)malloc(MAX_BATCH_VERTICES * sizeof(BatchVertex));
    if (!renderer->vertices) {
        fprintf(stderr, "Failed to allocate batch vertex buffer\n");
        exit(1);
    }

    glGenVertexArrays(1, &renderer->vao);
    glGenBuffers(1, &renderer->vbo);
    
    glBindVertexArray(renderer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    
    glBufferData(GL_ARRAY_BUFFER, 
                 MAX_BATCH_VERTICES * sizeof(BatchVertex),
                 NULL, 
                 GL_DYNAMIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 
                         sizeof(BatchVertex),
                         (void*)offsetof(BatchVertex, position));
    glEnableVertexAttribArray(0);

    // TexCoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                         sizeof(BatchVertex),
                         (void*)offsetof(BatchVertex, texCoords));
    glEnableVertexAttribArray(1);

    // Color attribute
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE,
                         sizeof(BatchVertex),
                         (void*)offsetof(BatchVertex, color));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    renderer->vertexCount = 0;
    renderer->initialized = true;

 //   printf("Batch renderer initialized with buffer size: %zu bytes\n",
 //          MAX_BATCH_VERTICES * sizeof(BatchVertex));
}
/**
 * @brief Adds a quad to the batch for rendering.
 * 
 * @param ctx The UI context containing the batch renderer.
 * @param x1, y1 Coordinates of the first corner of the quad.
 * @param x2, y2 Coordinates of the opposite corner of the quad.
 * @param u1, v1 Texture coordinates for the first corner.
 * @param u2, v2 Texture coordinates for the opposite corner.
 * @param color The color of the quad as a UIVec4f.
 * 
 * Divides the quad into two triangles and adds their vertex data to the batch. 
 * If the batch is near capacity, it flushes the batch before adding.
 */
void UI_BatchQuad(UIContext* ctx, 
                  float x1, float y1, float x2, float y2,
                  float u1, float v1, float u2, float v2,
                  UIVec4f color,
                  bool isTextured) {

    // Only flush in two cases:
    // 1. Buffer is full
    // 2. Texture state is changing
    bool needsFlush = false;
    
    if (ctx->batchRenderer.vertexCount >= MAX_BATCH_VERTICES - 6) {
        needsFlush = true;
        //printf("DEBUG: Flushing batch: buffer full\n");
    } else if (ctx->batchRenderer.vertexCount > 0) {
        bool currentBatchTextured = (ctx->batchRenderer.vertices[0].texCoords[0] != 0.0f || 
                                   ctx->batchRenderer.vertices[0].texCoords[1] != 0.0f);
        if (currentBatchTextured != isTextured) {
            needsFlush = true;
            //printf("DEBUG: Flushing batch: texture state change from %d to %d\n", 
             //      currentBatchTextured, isTextured);
        }
    }

    if (needsFlush) {
        UI_FlushBatch(ctx);
    }

    //printf("DEBUG: Adding to batch: textured=%d, vertices=%d\n", 
    //       isTextured, ctx->batchRenderer.vertexCount);

    BatchVertex* v = &ctx->batchRenderer.vertices[ctx->batchRenderer.vertexCount];

    // First triangle
    v[0].position[0] = x1; v[0].position[1] = y1;
    v[0].texCoords[0] = u1; v[0].texCoords[1] = v2; // Swapped v1 to v2
    v[0].color[0] = color.r; v[0].color[1] = color.g; 
    v[0].color[2] = color.b; v[0].color[3] = color.a;

    v[1].position[0] = x2; v[1].position[1] = y1;
    v[1].texCoords[0] = u2; v[1].texCoords[1] = v2; // Swapped v1 to v2
    v[1].color[0] = color.r; v[1].color[1] = color.g; 
    v[1].color[2] = color.b; v[1].color[3] = color.a;

    v[2].position[0] = x1; v[2].position[1] = y2;
    v[2].texCoords[0] = u1; v[2].texCoords[1] = v1; // Swapped v2 to v1
    v[2].color[0] = color.r; v[2].color[1] = color.g; 
    v[2].color[2] = color.b; v[2].color[3] = color.a;

    // Second triangle
    v[3].position[0] = x2; v[3].position[1] = y1;
    v[3].texCoords[0] = u2; v[3].texCoords[1] = v2; // Swapped v1 to v2
    v[3].color[0] = color.r; v[3].color[1] = color.g; 
    v[3].color[2] = color.b; v[3].color[3] = color.a;

    v[4].position[0] = x2; v[4].position[1] = y2;
    v[4].texCoords[0] = u2; v[4].texCoords[1] = v1; // Swapped v2 to v1
    v[4].color[0] = color.r; v[4].color[1] = color.g; 
    v[4].color[2] = color.b; v[4].color[3] = color.a;

    v[5].position[0] = x1; v[5].position[1] = y2;
    v[5].texCoords[0] = u1; v[5].texCoords[1] = v1; // Swapped v2 to v1
    v[5].color[0] = color.r; v[5].color[1] = color.g; 
    v[5].color[2] = color.b; v[5].color[3] = color.a;

    ctx->batchRenderer.vertexCount += 6;
}

/**
 * @brief Prepares the UI batch renderer for drawing.
 * 
 * @param ctx The UI context containing the batch renderer.
 * 
 * Sets up OpenGL state for blending and binds the texture atlas. Resets
 * the vertex count for the current batch.
 */
void UI_BeginBatch(UIContext* ctx) {
    if (!ctx || !ctx->batchRenderer.initialized) {
        printf("ERROR: Invalid context or uninitialized batch renderer\n");
        return;
    }

    //printf("\nDEBUG: === BATCH BEGIN STATE ===\n");
    
    // Check initial GL state
    GLint previousVAO, previousProgram;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVAO);
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    //printf("DEBUG: Initial GL State - VAO: %d, Program: %d\n", previousVAO, previousProgram);

    // Set and verify shader program
    glUseProgram(ctx->shaderProgram);
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    //printf("DEBUG: Setting shader program ID: %d (Verified: %d)\n", 
    //       ctx->shaderProgram, currentProgram);

    // Explicitly bind VAO and verify
    glBindVertexArray(ctx->batchRenderer.vao);
    GLint boundVAO;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &boundVAO);
    //printf("DEBUG: Binding VAO ID: %d (Verified: %d)\n", 
    //       ctx->batchRenderer.vao, boundVAO);

    // Verify attribute enables after VAO binding
    GLint positionEnabled, texCoordEnabled, colorEnabled;
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &positionEnabled);
    glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &texCoordEnabled);
    glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &colorEnabled);
    //printf("DEBUG: Attribute States - Position: %d, TexCoord: %d, Color: %d\n",
    //       positionEnabled, texCoordEnabled, colorEnabled);

    // Set up blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Texture setup with verification
    glActiveTexture(GL_TEXTURE0);
    GLint activeTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
    // printf("DEBUG: Active Texture Unit: %d (Expected: %d)\n", 
    //       activeTexture - GL_TEXTURE0, 0);

    glBindTexture(GL_TEXTURE_2D, textureAtlas);
    GLint boundTexture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTexture);
    // printf("DEBUG: Binding texture atlas ID: %d (Verified: %d)\n", 
    //       textureAtlas, boundTexture);

    // Set and verify uniforms
    GLint textureLoc = glGetUniformLocation(ctx->shaderProgram, "textureAtlas");
    if (textureLoc >= 0) {
        glUniform1i(textureLoc, 0);
        // printf("DEBUG: Setting texture uniform location: %d\n", textureLoc);
    } else {
        // printf("WARNING: textureAtlas uniform not found\n");
    }

    GLint hasTextureLoc = glGetUniformLocation(ctx->shaderProgram, "uHasTexture");
    if (hasTextureLoc >= 0) {
        glUniform1i(hasTextureLoc, 0);  // Default to no texture
        GLint verifiedValue;
        glGetUniformiv(ctx->shaderProgram, hasTextureLoc, &verifiedValue);
        // printf("DEBUG: Setting uHasTexture uniform: %d (Verified: %d)\n", 
        //       0, verifiedValue);
    } else {
        // printf("WARNING: uHasTexture uniform not found\n");
    }

    // Reset vertex count
    ctx->batchRenderer.vertexCount = 0;

    // Check for any GL errors
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        // printf("ERROR: GL error in BeginBatch: 0x%x\n", err);
    }

    // printf("DEBUG: === BATCH BEGIN COMPLETE ===\n\n");
}

/**
 * @brief Flushes the current batch of vertices to the GPU.
 * 
 * @param ctx The UI context containing the batch renderer.
 * 
 * Uploads the current batch of vertices to the GPU and renders them as
 * triangles. Resets the vertex count to prepare for the next batch.
 */
void UI_FlushBatch(UIContext* ctx) {
    if (!ctx || ctx->batchRenderer.vertexCount == 0) {
        printf("UI_FlushBatch: No vertices to flush.\n");
        return;
    }

    glUseProgram(ctx->shaderProgram);
    glBindVertexArray(ctx->batchRenderer.vao);

    BatchVertex* firstVertex = &ctx->batchRenderer.vertices[0];
    bool isTexturedBatch = (firstVertex->texCoords[0] != 0.0f || 
                           firstVertex->texCoords[1] != 0.0f);
    
    GLint hasTextureLoc = glGetUniformLocation(ctx->shaderProgram, "uHasTexture");
    glUniform1i(hasTextureLoc, isTexturedBatch ? 1 : 0);

    glBindBuffer(GL_ARRAY_BUFFER, ctx->batchRenderer.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                   ctx->batchRenderer.vertexCount * sizeof(BatchVertex),
                   ctx->batchRenderer.vertices);

    glDrawArrays(GL_TRIANGLES, 0, ctx->batchRenderer.vertexCount);
    ctx->batchRenderer.vertexCount = 0;
}
/**
 * @brief Ends the current batch rendering process.
 * 
 * @param ctx The UI context containing the batch renderer.
 * 
 * Flushes any remaining vertices in the batch, then disables OpenGL 
 * state for blending and unbinds resources.
 */
void UI_EndBatch(UIContext* ctx) {
    if (!ctx) {
        printf("ERROR: Invalid context in EndBatch\n");
        return;
    }

    // printf("\nDEBUG: === BATCH END STATE ===\n");

    // Verify final state before cleanup
    GLint currentVAO, currentProgram, currentTexture;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexture);
    /*
    printf("DEBUG: Final GL State - VAO: %d, Program: %d, Texture: %d\n",
           currentVAO, currentProgram, currentTexture);
*/
    UI_FlushBatch(ctx);

    // Verify attribute states before unbinding
    GLint positionEnabled, texCoordEnabled, colorEnabled;
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &positionEnabled);
    glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &texCoordEnabled);
    glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &colorEnabled);
 /*   
    printf("DEBUG: Final Attribute States - Position: %d, TexCoord: %d, Color: %d\n",
           positionEnabled, texCoordEnabled, colorEnabled);
*/
    glDisable(GL_BLEND);
    
    // Explicit cleanup of bindings
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
    
    // Verify clean state
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    
    // printf("DEBUG: Cleanup State - VAO: %d, Program: %d\n",
    //       currentVAO, currentProgram);

    // Check for any GL errors
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
    //    printf("ERROR: GL error in EndBatch: 0x%x\n", err);
    }

    // printf("DEBUG: === BATCH END COMPLETE ===\n\n");
}

/**
 * @brief Renders a single UI element.
 * 
 * @param ctx The UI context containing rendering information.
 * @param element A pointer to the UI element to be rendered.
 * 
 * Renders the background, border, and content of the element based on its 
 * type (e.g., progress bar, grid). Handles coordinate transformations for 
 * proper screen rendering.
 */
void UI_RenderElement(UIContext* ctx, UIElement* element) {
    if (!element) return;

    float x1 = (2.0f * element->position.x / ctx->viewportWidth) - 1.0f;
    float y1 = 1.0f - (2.0f * element->position.y / ctx->viewportHeight);
    float x2 = x1 + (2.0f * element->size.x / ctx->viewportWidth);
    float y2 = y1 - (2.0f * element->size.y / ctx->viewportHeight);

    if (element->type == ELEMENT_BAR) {
        // Draw background
        UI_BatchQuad(ctx, x1, y1, x2, y2, 
                    0.0f, 0.0f, 0.0f, 0.0f, 
                    element->backgroundColor, false);

        // Draw progress
        float progress = element->specific.bar.value / element->specific.bar.maxValue;
        float progressX2 = x1 + (x2 - x1) * progress;
        UIVec4f progressColor = {0.2f, 0.7f, 0.2f, 1.0f};
        
        UI_BatchQuad(ctx, x1, y1, progressX2, y2,
                    0.0f, 0.0f, 0.0f, 0.0f,
                    progressColor, false);
    }
    else if (element->type == ELEMENT_GRID) {
        // Draw container background
        UI_BatchQuad(ctx, x1, y1, x2, y2, 
                    0.0f, 0.0f, 0.0f, 0.0f, 
                    element->backgroundColor, false);

        float cellWidth = element->size.x / element->specific.grid.cols;
        float cellHeight = element->size.y / element->specific.grid.rows;

        for (int y = 0; y < element->specific.grid.rows; y++) {
            for (int x = 0; x < element->specific.grid.cols; x++) {
                int idx = y * element->specific.grid.cols + x;
                UICellData* cell = &element->specific.grid.cells[idx];

                float cx1 = element->position.x + x * cellWidth;
                float cy1 = element->position.y + y * cellHeight;

                float ncx1 = (2.0f * cx1 / ctx->viewportWidth) - 1.0f;
                float ncy1 = 1.0f - (2.0f * cy1 / ctx->viewportHeight);
                float ncx2 = ncx1 + (2.0f * cellWidth / ctx->viewportWidth);
                float ncy2 = ncy1 - (2.0f * cellHeight / ctx->viewportHeight);

                // Draw cell background
                UIVec4f cellColor = {0.2f, 0.2f, 0.2f, 0.9f};
                UI_BatchQuad(ctx, ncx1, ncy1, ncx2, ncy2, 
                           0.0f, 0.0f, 0.0f, 0.0f, 
                           cellColor, false);

                // Draw item if present
                if (cell->item) {
                    float padding = 4.0f / ctx->viewportWidth;
                    ncx1 += padding;
                    ncx2 -= padding;
                    ncy1 -= padding;
                    ncy2 += padding;

                    TextureCoords* texCoords = NULL;
                    switch(cell->item->type) {
                        case ITEM_FERN:
                            texCoords = getTextureCoords("item_fern");
                            break;
                        default:
                            break;
                    }

                    if (texCoords) {
                        float cellWidthPixels = (ncx2 - ncx1) * ctx->viewportWidth * 0.5f;
                        float cellHeightPixels = (ncy1 - ncy2) * ctx->viewportHeight * 0.5f;
                        
                        float textureAspect = ((texCoords->u2 - texCoords->u1) * ATLAS_COLS) / 
                                            ((texCoords->v2 - texCoords->v1) * ATLAS_ROWS);
                        float cellAspect = cellWidthPixels / cellHeightPixels;
                        
                        if (cellAspect > textureAspect) {
                            float desiredWidth = cellHeightPixels * textureAspect;
                            float excess = cellWidthPixels - desiredWidth;
                            float normalizedExcess = excess / (ctx->viewportWidth * 0.5f);
                            ncx1 += normalizedExcess * 0.5f;
                            ncx2 -= normalizedExcess * 0.5f;
                        } else {
                            float desiredHeight = cellWidthPixels / textureAspect;
                            float excess = cellHeightPixels - desiredHeight;
                            float normalizedExcess = excess / (ctx->viewportHeight * 0.5f);
                            ncy1 -= normalizedExcess * 0.5f;
                            ncy2 += normalizedExcess * 0.5f;
                        }

                        UI_BatchQuad(ctx, ncx1, ncy1, ncx2, ncy2,
                                   texCoords->u1, texCoords->v1,
                                   texCoords->u2, texCoords->v2,
                                   (UIVec4f){1.0f, 1.0f, 1.0f, 1.0f},
                                   true);
                    }
                }
            }
        }
    }
    else if (element->type == ELEMENT_CONTAINER) {
        // Draw container background
        UI_BatchQuad(ctx, x1, y1, x2, y2,
                    0.0f, 0.0f, 0.0f, 0.0f,
                    element->backgroundColor, false);
    }
}
/**
 * @brief Creates a new UI element of a specified type.
 * 
 * @param type The type of the UI element to create.
 * @return A pointer to the newly created UI element, or NULL if allocation fails.
 * 
 * Initializes the UI element with default values for appearance and behavior.
 */
UIElement* UI_CreateElement(ElementType type) {
    UIElement* element = calloc(1, sizeof(UIElement));
    if (!element) {
        fprintf(stderr, "Failed to allocate UI element\n");
        return NULL;
    }

    element->type = type;
    element->borderWidth = 1.0f;
    element->backgroundColor = (UIVec4f){0.2f, 0.2f, 0.2f, 0.9f};
    element->borderColor = (UIVec4f){0.4f, 0.4f, 0.4f, 1.0f};

    return element;
}

/**
 * @brief Creates a new progress bar element.
 * 
 * @return A pointer to the newly created progress bar element, or NULL if allocation fails.
 * 
 * Initializes the progress bar with default properties such as size, color, 
 * and range. Logs creation details for debugging.
 */
UIElement* UI_CreateProgressBar(void) {
    UIElement* bar = UI_CreateElement(ELEMENT_BAR);
    if (!bar) return NULL;

    // Specialized bar setup
    bar->specific.bar.value = 0.0f;
    bar->specific.bar.maxValue = 1.0f;
    bar->specific.bar.flashIntensity = 0.0f;
    bar->backgroundColor = (UIVec4f){0.1f, 0.1f, 0.1f, 0.9f};
    bar->borderColor = (UIVec4f){0.5f, 0.5f, 0.5f, 1.0f};
    bar->size = (UIVec2f){200.0f, 20.0f};

    printf("Created progress bar: %p\n", (void*)bar);
    return bar;
}

/**
 * @brief Creates a new grid element with specified dimensions.
 * 
 * @param rows The number of rows in the grid.
 * @param cols The number of columns in the grid.
 * @return A pointer to the newly created grid element, or NULL if allocation fails.
 * 
 * Allocates and initializes the grid, including its individual cells.
 */
UIElement* UI_CreateGrid(int rows, int cols) {
    if (rows <= 0 || cols <= 0 || rows * cols > MAX_GRID_CELLS) {
        fprintf(stderr, "Invalid grid dimensions: %dx%d\n", rows, cols);
        return NULL;
    }

    UIElement* grid = UI_CreateElement(ELEMENT_GRID);
    if (!grid) return NULL;

    grid->specific.grid.rows = rows;
    grid->specific.grid.cols = cols;
    grid->specific.grid.cellAspectRatio = 1.0f;
    
    // Set a darker background for the grid container
    grid->backgroundColor = (UIVec4f){0.5f, 0.5f, 0.5f, 1.0f};  // Darker than cells

    printf("Created grid: %dx%d at %p\n", rows, cols, (void*)grid);

    // Initialize cells with a lighter color than the container
    for (int i = 0; i < rows * cols; i++) {
        grid->specific.grid.cells[i].hasItem = false;
        grid->specific.grid.cells[i].itemTexX = 0.0f;
        grid->specific.grid.cells[i].itemTexY = 0.0f;
    }

    return grid;
}

/**
 * @brief Updates a grid element with inventory data.
 * 
 * @param grid The grid element to update.
 * @param inv The inventory data to populate into the grid.
 * 
 * Maps inventory items to grid cells and updates their positions, sizes,
 * and texture coordinates. Logs updates for debugging.
 */
void UI_UpdateInventoryGrid(UIElement* grid, const Inventory* inv) {
    if (!grid || !inv || grid->type != ELEMENT_GRID) {
        printf("Error: Invalid grid or inventory\n");
        return;
    }

    // Reset all cells first
    for (int i = 0; i < (grid->specific.grid.rows * grid->specific.grid.cols); i++) {
        grid->specific.grid.cells[i].item = NULL;
        grid->specific.grid.cells[i].itemTexX = 0.0f;
        grid->specific.grid.cells[i].itemTexY = 0.0f;
    }

    float cellWidth = grid->size.x / grid->specific.grid.cols;
    float cellHeight = grid->size.y / grid->specific.grid.rows;

    // Only process valid slots
    for (int i = 0; i < inv->slotCount && 
         i < (grid->specific.grid.rows * grid->specific.grid.cols); i++) {
        
        const Item* item = inv->slots[i];
        int row = i / grid->specific.grid.cols;
        int col = i % grid->specific.grid.cols;
        UICellData* cell = &grid->specific.grid.cells[i];

        cell->position.x = grid->position.x + (col * cellWidth);
        cell->position.y = grid->position.y + (row * cellHeight);
        cell->size.x = cellWidth;
        cell->size.y = cellHeight;

        if (item) {
            cell->item = item;
            TextureCoords* texCoords = NULL;
            
            // Get texture coordinates based on item type
            switch(item->type) {
                case ITEM_FERN:
                    texCoords = getTextureCoords("item_fern");
                    break;
                default:
                    fprintf(stderr, "Unknown item type: %d\n", item->type);
                    continue;
            }

            if (texCoords) {
                cell->itemTexX = texCoords->u1;
                cell->itemTexY = texCoords->v1;
            }
        }
    }
}
/**
 * @brief Initializes the UI system.
 * 
 * @param shaderProgram The OpenGL shader program used for rendering.
 * 
 * Sets up the UI context, initializes the batch renderer, and creates
 * default UI elements such as the experience bar and inventory grid.
 */
void InitializeUI(GLuint shaderProgram) {
    printf("Initializing UI with shader: %u\n", shaderProgram);
    
    gContext = calloc(1, sizeof(UIContext));
    if (!gContext) {
        fprintf(stderr, "Failed to allocate UI context\n");
        return;
    }
    
    gContext->shaderProgram = shaderProgram;
    gContext->viewportWidth = SIDEBAR_WIDTH;
    gContext->viewportHeight = WINDOW_HEIGHT;
    
    UI_InitBatchRenderer(&gContext->batchRenderer);
    
    UIElement* expBar = UI_CreateProgressBar();
    if (expBar) {
        expBar->position = (UIVec2f){10.0f, 10.0f};
        expBar->size = (UIVec2f){SIDEBAR_WIDTH - 20.0f, 30.0f};
        uiState.expBar = expBar;
    }
    
    UIElement* inventory = UI_CreateGrid(5, 5);
    if (inventory) {
        inventory->position = (UIVec2f){10.0f, 50.0f};
        inventory->size = (UIVec2f){SIDEBAR_WIDTH - 20.0f, 400.0f};
        uiState.inventory = inventory;
    }
}

/**
 * @brief Renders the entire UI, including elements like progress bars and grids.
 * 
 * @param player The player whose data is displayed in the UI.
 * 
 * Updates and renders all UI elements based on the current player state.
 */
void RenderUI(const Player* player) {
    if (!gContext || !player) return;
    
    UI_BeginBatch(gContext);
   // printf("\n=== UI RENDER FRAME START ===\n");
    
    // Draw sidebar background first
    UI_RenderSidebarBackground(gContext);
    
if (uiState.expBar) {
    UIElement* expBar = (UIElement*)uiState.expBar;
    
    // Use the last updated skill
    SkillType skillToShow = player->skills.lastUpdatedSkill;
    float skillExp = player->skills.experience[skillToShow];
    uint32_t skillLevel = player->skills.levels[skillToShow];
    float currentLevelExp = skillExp - (skillLevel * EXP_PER_LEVEL);
    float progress = currentLevelExp / EXP_PER_LEVEL;
    
    expBar->specific.bar.value = progress;
    expBar->specific.bar.maxValue = 1.0f;
    
    UI_RenderElement(gContext, expBar);
    
    // Add some debug output to verify
   // printf("Rendering exp bar for skill %s: %.1f/%.1f (%.1f%%)\n", 
   //        getSkillName(skillToShow), 
     //      currentLevelExp, 
       //    EXP_PER_LEVEL, 
         //  progress * 100.0f);
}
    
    if (uiState.inventory) {
        UIElement* grid = (UIElement*)uiState.inventory;
       // printf("Found inventory grid: %dx%d\n", 
       //        grid->specific.grid.rows, 
         //      grid->specific.grid.cols);

        // Update grid with current inventory state
        UI_UpdateInventoryGrid(grid, player->inventory);
        
      //  printf("Rendering inventory grid...\n");
        UI_RenderElement(gContext, grid);
    } else {
        printf("No inventory grid found!\n");
    }
    
   // printf("=== UI RENDER FRAME END ===\n\n");
    UI_EndBatch(gContext);
}

/**
* @brief Renders the background of the sidebar UI.
*
* @param ctx Pointer to the UI context containing batch renderer and viewport information
*
* Renders a solid dark gray quad covering the entire sidebar viewport area. Uses normalized device 
* coordinates (-1 to 1) for the quad vertices since it should fill the entire viewport. The quad
* is rendered without texturing using just a solid color.
* 
* This function must be called before rendering other UI elements to provide the base background.
* Returns early if ctx is NULL.
*
* @warning Must be called between UI_BeginBatch() and UI_EndBatch() calls
*/void UI_RenderSidebarBackground(UIContext* ctx) {
    if (!ctx) return;

    // Use dark gray for sidebar background
    UIVec4f backgroundColor = {0.2f, 0.2f, 0.2f, 1.0f};

    // Render background quad covering entire sidebar (no texture)
    UI_BatchQuad(ctx, 
                 -1.0f, -1.0f,    // Bottom-left
                 1.0f, 1.0f,      // Top-right
                 0.0f, 0.0f,      // Tex coords (unused)
                 0.0f, 0.0f,      // Tex coords (unused)
                 backgroundColor,
                 false);          // Not textured
}

/**
 * @brief Destroys a UI element and frees its resources.
 * 
 * @param element The UI element to destroy.
 * 
 * Performs cleanup specific to the element's type and releases allocated memory.
 */
void UI_DestroyElement(UIElement* element) {
    if (!element) return;

    // Type-specific cleanup
    switch (element->type) {
        case ELEMENT_GRID:
            for (int i = 0; i < (element->specific.grid.rows * element->specific.grid.cols); i++) {
                // Don't free the items themselves - they're owned by the inventory
                element->specific.grid.cells[i].item = NULL;
            }
            break;
            
        case ELEMENT_BAR:
            // No additional resources to clean
            break;
            
        case ELEMENT_CONTAINER:
            // No additional resources to clean
            break;
            
        case ELEMENT_BUTTON:
            // No additional resources to clean
            break;
    }

    free(element);
    printf("Destroyed UI element: %p\n", (void*)element);
}
/**
 * @brief Cleans up the entire UI system.
 * 
 * Frees all allocated resources, including the UI context, batch renderer, 
 * and created elements. Resets the global UI state.
 */
void CleanupUI(void) {
    if (!gContext) return;
    
    if (uiState.expBar) {
        UI_DestroyElement((UIElement*)uiState.expBar);
        uiState.expBar = NULL;
    }
    
    if (uiState.inventory) {
        UI_DestroyElement((UIElement*)uiState.inventory);
        uiState.inventory = NULL;
    }
    
    if (gContext->batchRenderer.vertices) {
        free(gContext->batchRenderer.vertices);
    }
    
    if (gContext->batchRenderer.vao) {
        glDeleteVertexArrays(1, &gContext->batchRenderer.vao);
    }
    
    if (gContext->batchRenderer.vbo) {
        glDeleteBuffers(1, &gContext->batchRenderer.vbo);
    }
    
    free(gContext);
    gContext = NULL;
}