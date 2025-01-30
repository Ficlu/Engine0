
#include "overlay.h"
#include <math.h>
#include "texture_coords.h"
#include "inventory.h"
#include "storage.h"
#include <GL/glew.h>
#include "rendering.h"
#include <stdio.h>
GLuint crateUIShaderProgram = 0;  // Shader program for crate UI
GLint crateUIProjLoc = 0;     
CrateUIRenderer gCrateUIRenderer = {0};

CrateUIProjection crateUIProj;

void mat4_ortho(mat4 result, float left, float right, float bottom, float top, float near, float far) {
    float rml = right - left;
    float tmb = top - bottom;
    float fmn = far - near;

    // Avoid division by zero
    if (fabsf(rml) < 0.0001f || fabsf(tmb) < 0.0001f || fabsf(fmn) < 0.0001f) {
        memset(result, 0, sizeof(mat4));
        result[0][0] = 1.0f;
        result[1][1] = 1.0f;
        result[2][2] = 1.0f;
        result[3][3] = 1.0f;
        return;
    }

    result[0][0] = 2.0f / rml;
    result[0][1] = 0.0f;
    result[0][2] = 0.0f;
    result[0][3] = 0.0f;

    result[1][0] = 0.0f;
    result[1][1] = 2.0f / tmb;
    result[1][2] = 0.0f;
    result[1][3] = 0.0f;

    result[2][0] = 0.0f;
    result[2][1] = 0.0f;
    result[2][2] = -2.0f / fmn;
    result[2][3] = 0.0f;

    result[3][0] = -(right + left) / rml;
    result[3][1] = -(top + bottom) / tmb;
    result[3][2] = -(far + near) / fmn;
    result[3][3] = 1.0f;
}


void RenderCrateUI(CrateInventory* crate, float screenX, float screenY) {
   if (!crate || !crate->isOpen || !gCrateUIRenderer.initialized) return;

   // Save current viewport
   GLint currentViewport[4];
   glGetIntegerv(GL_VIEWPORT, currentViewport);

   // Set to game viewport for crate UI
   glViewport(gameViewport.x, gameViewport.y, gameViewport.width, gameViewport.height);

   glUseProgram(gCrateUIRenderer.shaderProgram);
   glBindVertexArray(gCrateUIRenderer.vao);
   glBindBuffer(GL_ARRAY_BUFFER, gCrateUIRenderer.vbo);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   // Use simpler orthographic projection 
   float left = 0.0f;
   float right = (float)gameViewport.width;
   float bottom = (float)gameViewport.height;
   float top = 0.0f;
   float near = -1.0f;
   float far = 1.0f;

   mat4 projection;
   mat4_ortho(projection, left, right, bottom, top, near, far);

   GLint projLoc = glGetUniformLocation(gCrateUIRenderer.shaderProgram, "projection");
   glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);

   // UI dimensions

   const float SLOT_SIZE = 48.0f;
   const float ITEM_SIZE = 32.0f;
   const float PADDING = 16.0f;

   // Calculate how many slots can fit in a row
   const int SLOTS_PER_ROW = (int)((CRATE_UI_WIDTH - PADDING) / (SLOT_SIZE + PADDING));

   // Calculate centered position
   float centeredX = ((float)gameViewport.width - CRATE_UI_WIDTH) * 0.5f;
   float centeredY = ((float)gameViewport.height - CRATE_UI_HEIGHT) * 0.5f;

   // Use these instead of incoming coordinates
   screenX = centeredX;
   screenY = centeredY;

   float bgVertices[] = {
       // First triangle
       screenX,                    screenY,                    0.0f, 0.0f,
       screenX + CRATE_UI_WIDTH,   screenY,                    0.0f, 0.0f,
       screenX + CRATE_UI_WIDTH,   screenY + CRATE_UI_HEIGHT,  0.0f, 0.0f,
       // Second triangle
       screenX,                    screenY,                    0.0f, 0.0f,
       screenX + CRATE_UI_WIDTH,   screenY + CRATE_UI_HEIGHT,  0.0f, 0.0f,
       screenX,                    screenY + CRATE_UI_HEIGHT,  0.0f, 0.0f
   };

   glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bgVertices), bgVertices);
   
   GLint colorLoc = glGetUniformLocation(gCrateUIRenderer.shaderProgram, "color");
   GLint hasTextureLoc = glGetUniformLocation(gCrateUIRenderer.shaderProgram, "hasTexture");

   glUniform4f(colorLoc, 0.2f, 0.2f, 0.2f, 0.9f);
   glUniform1i(hasTextureLoc, 0);
   glDrawArrays(GL_TRIANGLES, 0, 6);

   // Bind texture atlas for item rendering
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, textureAtlas);
   GLint texLoc = glGetUniformLocation(gCrateUIRenderer.shaderProgram, "textureAtlas");
   glUniform1i(texLoc, 0);

   // Render item slots
   int totalSlotsRendered = 0;
   float currentY = screenY + PADDING;

   for (int i = 0; i < MATERIAL_COUNT; i++) {
       if (isPlantMaterial(i) && crate->items[i].count > 0) {
           printf("Processing material %d with count: %d\n", i, crate->items[i].count);

           TextureCoords* itemTex = NULL;
           switch (i) {
               case MATERIAL_FERN:
                   itemTex = getTextureCoords("item_fern");
                   break;
               case MATERIAL_TREE:
                   itemTex = getTextureCoords("tree_trunk");
                   break;
           }

           if (itemTex) {
               for (int itemIdx = 0; itemIdx < crate->items[i].count; itemIdx++) {
                   // Calculate grid position
                   int row = totalSlotsRendered / SLOTS_PER_ROW;
                   int col = totalSlotsRendered % SLOTS_PER_ROW;

                   float itemSlotX = screenX + PADDING + col * (SLOT_SIZE + PADDING);
                   float itemSlotY = screenY + PADDING + row * (SLOT_SIZE + PADDING);
                   
                   printf("Rendering item %d at position (%f, %f), row %d, col %d\n", 
                          totalSlotsRendered, itemSlotX, itemSlotY, row, col);

                   // Draw slot background
                   float slotVertices[] = {
                       // First triangle
                       itemSlotX,               itemSlotY,                  0.0f, 0.0f,
                       itemSlotX + SLOT_SIZE,   itemSlotY,                  0.0f, 0.0f,
                       itemSlotX + SLOT_SIZE,   itemSlotY + SLOT_SIZE,      0.0f, 0.0f,
                       // Second triangle
                       itemSlotX,               itemSlotY,                  0.0f, 0.0f,
                       itemSlotX + SLOT_SIZE,   itemSlotY + SLOT_SIZE,      0.0f, 0.0f,
                       itemSlotX,               itemSlotY + SLOT_SIZE,      0.0f, 0.0f
                   };

                   glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(slotVertices), slotVertices);
                   glUniform4f(colorLoc, 0.3f, 0.3f, 0.3f, 1.0f);
                   glUniform1i(hasTextureLoc, 0);
                   glDrawArrays(GL_TRIANGLES, 0, 6);

                   // Draw the item centered in its slot
                   float itemStartX = itemSlotX + (SLOT_SIZE - ITEM_SIZE) / 2;
                   float itemStartY = itemSlotY + (SLOT_SIZE - ITEM_SIZE) / 2;

                   float itemVertices[] = {
                       // First triangle
                       itemStartX,              itemStartY,               itemTex->u1, itemTex->v2,
                       itemStartX + ITEM_SIZE,  itemStartY,               itemTex->u2, itemTex->v2,
                       itemStartX + ITEM_SIZE,  itemStartY + ITEM_SIZE,   itemTex->u2, itemTex->v1,
                       
                       // Second triangle
                       itemStartX,              itemStartY,               itemTex->u1, itemTex->v2,
                       itemStartX + ITEM_SIZE,  itemStartY + ITEM_SIZE,   itemTex->u2, itemTex->v1,
                       itemStartX,              itemStartY + ITEM_SIZE,   itemTex->u1, itemTex->v1
                   };

                   glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(itemVertices), itemVertices);
                   glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
                   glUniform1i(hasTextureLoc, 1);
                   glDrawArrays(GL_TRIANGLES, 0, 6);

                   totalSlotsRendered++;
               }
           }
       }
   }

   // Restore previous viewport
   glViewport(currentViewport[0], currentViewport[1], currentViewport[2], currentViewport[3]);

   // Cleanup GL state
   glDisable(GL_BLEND);
   glBindVertexArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glUseProgram(0);
}
void RenderCrateUIs(float cameraOffsetX, float cameraOffsetY, float zoomFactor) {
    printf("Starting RenderCrateUIs, storage count: %zu\n", globalStorageManager.count);
    for (size_t i = 0; i < globalStorageManager.count; i++) {
        CrateInventory* crate = &globalStorageManager.crates[i];
        if (crate->isOpen) {
            printf("Found open crate at index %zu\n", i);
            int gridX = crate->crateId % GRID_SIZE;
            int gridY = crate->crateId / GRID_SIZE;
            
            float worldX, worldY;
            WorldToScreenCoords(gridX, gridY, cameraOffsetX, cameraOffsetY, zoomFactor, &worldX, &worldY);
            
            float screenX = (worldX + 1.0f) * GAME_VIEW_WIDTH / 2.0f;
            float screenY = (1.0f - worldY) * WINDOW_HEIGHT / 2.0f;
            
            printf("Rendering crate UI at screen coords: (%f, %f)\n", screenX, screenY);
            RenderCrateUI(crate, screenX, screenY);
        }
    }
}
GLuint createCrateUIShaderProgram(void) {
    // Create vertex shader
    GLuint vertexShader = createShader(GL_VERTEX_SHADER, crateUIVertexShader);
    if (!vertexShader) {
        fprintf(stderr, "Failed to create crate UI vertex shader\n");
        return 0;
    }

    // Create fragment shader
    GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, crateUIFragmentShader);
    if (!fragmentShader) {
        glDeleteShader(vertexShader);
        fprintf(stderr, "Failed to create crate UI fragment shader\n");
        return 0;
    }

    // Create shader program
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // Check link status
    GLint success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "Failed to link crate UI shader program: %s\n", infoLog);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(program);
        return 0;
    }

    // Clean up shader objects
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Get projection uniform location right away
    crateUIProjLoc = glGetUniformLocation(program, "projection");
    if (crateUIProjLoc == -1) {
        fprintf(stderr, "Warning: Could not find 'projection' uniform in crate UI shader\n");
    }

    return program;
}

void beginCrateUIRender(void) {
    glUseProgram(gCrateUIRenderer.shaderProgram);
    
    mat4 projection;
    mat4_ortho(projection,
               crateUIProj.left, crateUIProj.right,
               crateUIProj.bottom, crateUIProj.top,
               crateUIProj.near, crateUIProj.far);
    
    GLint projLoc = glGetUniformLocation(gCrateUIRenderer.shaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void renderCrateUIOverlay(CrateInventory* crate, float screenX, float screenY) {
    if (!crate || !gCrateUIRenderer.initialized) return;


    const float SLOT_SIZE = 32.0f;
    const float PADDING = 8.0f;

    // Begin crate UI rendering
    glUseProgram(gCrateUIRenderer.shaderProgram);
    glBindVertexArray(gCrateUIRenderer.vao);

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    mat4 projection;
    mat4_ortho(projection,
               gCrateUIRenderer.projection.left,
               gCrateUIRenderer.projection.right,
               gCrateUIRenderer.projection.bottom,
               gCrateUIRenderer.projection.top,
               gCrateUIRenderer.projection.near,
               gCrateUIRenderer.projection.far);
    
    GLint projLoc = glGetUniformLocation(gCrateUIRenderer.shaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);

    // Render background panel
    float bgVertices[] = {
        // First triangle
        screenX,                  screenY,                   0.0f, 0.0f,
        screenX + CRATE_UI_WIDTH, screenY,                   0.0f, 0.0f,
        screenX + CRATE_UI_WIDTH, screenY + CRATE_UI_HEIGHT, 0.0f, 0.0f,
        // Second triangle
        screenX,                  screenY,                   0.0f, 0.0f,
        screenX + CRATE_UI_WIDTH, screenY + CRATE_UI_HEIGHT, 0.0f, 0.0f,
        screenX,                  screenY + CRATE_UI_HEIGHT, 0.0f, 0.0f
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(bgVertices), bgVertices, GL_DYNAMIC_DRAW);
    glUniform4f(glGetUniformLocation(gCrateUIRenderer.shaderProgram, "color"), 0.2f, 0.2f, 0.2f, 0.9f);
    glUniform1i(glGetUniformLocation(gCrateUIRenderer.shaderProgram, "hasTexture"), 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Bind texture atlas for item rendering
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureAtlas);
    glUniform1i(glGetUniformLocation(gCrateUIRenderer.shaderProgram, "textureAtlas"), 0);

    // Render item slots
    float currentY = screenY + PADDING;
    for (int i = 0; i < MATERIAL_COUNT; i++) {
        if (isPlantMaterial(i) && crate->items[i].count > 0) {
            // Render slot background
            float slotVertices[] = {
                // First triangle
                screenX + PADDING,           currentY,                  0.0f, 0.0f,
                screenX + PADDING + SLOT_SIZE, currentY,                  0.0f, 0.0f,
                screenX + PADDING + SLOT_SIZE, currentY + SLOT_SIZE,      0.0f, 0.0f,
                // Second triangle
                screenX + PADDING,           currentY,                  0.0f, 0.0f,
                screenX + PADDING + SLOT_SIZE, currentY + SLOT_SIZE,      0.0f, 0.0f,
                screenX + PADDING,           currentY + SLOT_SIZE,      0.0f, 0.0f
            };

            glBufferData(GL_ARRAY_BUFFER, sizeof(slotVertices), slotVertices, GL_DYNAMIC_DRAW);
            glUniform4f(glGetUniformLocation(gCrateUIRenderer.shaderProgram, "color"), 0.3f, 0.3f, 0.3f, 1.0f);
            glUniform1i(glGetUniformLocation(gCrateUIRenderer.shaderProgram, "hasTexture"), 0);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Get item texture coordinates
            TextureCoords* itemTex = NULL;
            switch (i) {
                case MATERIAL_FERN:
                    itemTex = getTextureCoords("item_fern");
                    break;
                case MATERIAL_TREE:
                    itemTex = getTextureCoords("tree_trunk");
                    break;
            }

            if (itemTex) {
                // Add slight padding to the item within the slot
                const float ITEM_PADDING = 2.0f;
                float itemVertices[] = {
                    // First triangle
                    screenX + PADDING + ITEM_PADDING,           currentY + ITEM_PADDING,                itemTex->u1, itemTex->v1,
                    screenX + PADDING + SLOT_SIZE - ITEM_PADDING, currentY + ITEM_PADDING,                itemTex->u2, itemTex->v1,
                    screenX + PADDING + SLOT_SIZE - ITEM_PADDING, currentY + SLOT_SIZE - ITEM_PADDING,    itemTex->u2, itemTex->v2,
                    // Second triangle
                    screenX + PADDING + ITEM_PADDING,           currentY + ITEM_PADDING,                itemTex->u1, itemTex->v1,
                    screenX + PADDING + SLOT_SIZE - ITEM_PADDING, currentY + SLOT_SIZE - ITEM_PADDING,    itemTex->u2, itemTex->v2,
                    screenX + PADDING + ITEM_PADDING,           currentY + SLOT_SIZE - ITEM_PADDING,    itemTex->u1, itemTex->v2
                };

                glBufferData(GL_ARRAY_BUFFER, sizeof(itemVertices), itemVertices, GL_DYNAMIC_DRAW);
                glUniform4f(glGetUniformLocation(gCrateUIRenderer.shaderProgram, "color"), 1.0f, 1.0f, 1.0f, 1.0f);
                glUniform1i(glGetUniformLocation(gCrateUIRenderer.shaderProgram, "hasTexture"), 1);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }

            currentY += SLOT_SIZE + PADDING;
        }
    }

    // Cleanup
    glDisable(GL_BLEND);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}

void initializeCrateUIRenderer(void) {
    if (gCrateUIRenderer.initialized) {
        printf("Crate UI renderer already initialized\n");
        return;
    }
    printf("Initializing crate UI renderer...\n");

    gCrateUIRenderer.shaderProgram = createCrateUIShaderProgram();
    
    glGenVertexArrays(1, &gCrateUIRenderer.vao);
    glGenBuffers(1, &gCrateUIRenderer.vbo);
    
    // Allocate buffer with enough space for background and slots
    glBindBuffer(GL_ARRAY_BUFFER, gCrateUIRenderer.vbo);
    glBufferData(GL_ARRAY_BUFFER, 1024 * sizeof(float), NULL, GL_DYNAMIC_DRAW); // Larger size

    glBindVertexArray(gCrateUIRenderer.vao);
    
    // Position attrib
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texcoord attrib 
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // Initialize projection
    gCrateUIRenderer.projection.left = 0.0f;
    gCrateUIRenderer.projection.right = GAME_VIEW_WIDTH;
    gCrateUIRenderer.projection.bottom = WINDOW_HEIGHT;
    gCrateUIRenderer.projection.top = 0.0f;

    gCrateUIRenderer.initialized = true;
        printf("Crate UI renderer initialization complete\n");

}



void cleanupCrateUIRenderer(void) {
    if (!gCrateUIRenderer.initialized) return;
    
    printf("Cleaning up crate UI renderer...\n");
    
    // Delete shader program
    if (gCrateUIRenderer.shaderProgram) {
        glDeleteProgram(gCrateUIRenderer.shaderProgram);
        gCrateUIRenderer.shaderProgram = 0;
    }
    
    // Delete VAO
    if (gCrateUIRenderer.vao) {
        glDeleteVertexArrays(1, &gCrateUIRenderer.vao);
        gCrateUIRenderer.vao = 0;
    }
    
    // Delete VBO
    if (gCrateUIRenderer.vbo) {
        glDeleteBuffers(1, &gCrateUIRenderer.vbo);
        gCrateUIRenderer.vbo = 0;
    }
    
    gCrateUIRenderer.initialized = false;
    printf("Crate UI renderer cleanup complete.\n");
}