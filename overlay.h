#include <math.h>
#include "texture_coords.h"
#include "inventory.h"
#include "storage.h"
#include <GL/glew.h>
#define CRATE_UI_WIDTH 400.0f
#define CRATE_UI_HEIGHT 600.0f
typedef struct {
    float left, right;
    float bottom, top;
    float near, far;
} CrateUIProjection;

typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint shaderProgram;
    CrateUIProjection projection;
    bool initialized;
} CrateUIRenderer;

extern CrateUIRenderer gCrateUIRenderer;
extern CrateUIProjection crateUIProj;

// Add these function declarations:
GLuint createCrateUIShaderProgram(void);
void cleanupCrateUIRenderer(void);
typedef float mat4[4][4];  // 4x4 matrix type

// Add these function declarations
void mat4_ortho(mat4 result, float left, float right, float bottom, 
                float top, float near, float far);
void initializeCrateUIRenderer(void);
void beginCrateUIRender(void);
void endCrateUIRender(void);
void renderCrateUIOverlay(CrateInventory* crate, float screenX, float screenY);
void cleanupStorageManager(StorageManager* manager);
void RenderCrateUI(CrateInventory* crate, float screenX, float screenY);
void RenderCrateUIs(float cameraOffsetX, float cameraOffsetY, float zoomFactor);
