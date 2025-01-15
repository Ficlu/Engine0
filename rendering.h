#ifndef RENDERING_H
#define RENDERING_H

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <stdlib.h>
#include "enemy.h"
#include "gameloop.h"
#include "structures.h"

typedef struct {
    float* persistentBuffer;
    size_t bufferCapacity;
} EntityBatchData;

typedef struct {
    float* persistentBuffer;
    size_t bufferCapacity;
} TileBatchData;

typedef struct {
    int x;
    int y;
    int width;
    int height;
} Viewport;

// UI Resource getters
GLuint getUIShaderProgram(void);
GLuint getUIVAO(void);
GLuint getUIVBO(void);
void initUIResources(void);

void renderSidebarButton(float x, float y, float width, float height);

extern GLuint itemShaderProgram;
extern Viewport gameViewport;
extern Viewport sidebarViewport;

bool isPointInGameView(int x, int y);
bool isPointInSidebar(int x, int y);

void initializeViewports(void);
void applyViewport(const Viewport* viewport);

extern GLuint squareVAO;
extern GLuint squareVBO;
extern GLuint shaderProgram;
GLuint createUIShaderProgram(void); 
extern TileBatchData tileBatchData;

extern EntityBatchData entityBatchData;
void cleanupEntityBatchData(void);
void cleanupTileBatchData(void);

extern GLuint textureAtlas;
extern GLuint textureUniform;
extern GLuint enemyBatchVBO;
extern GLuint enemyBatchVAO;
extern GLuint outlineVBO;
void cleanupUIResources(void);
GLuint createItemShaderProgram(void);
void setupGameViewport(void);
void setupSidebarViewport(void);
void initRendering();
GLuint createShaderProgram();
void createGridVertices(float** vertices, int* vertexCount, int width, int height, int cellSize);
GLuint createGridVAO(float* vertices, int vertexCount);
GLuint createOutlineShaderProgram();
void initializeOutlineVAO();
GLuint createSquareVAO(float size, float texX, float texY, float texWidth, float texHeight);
GLuint loadBMP(const char* filePath);
GLuint createShader(GLenum type, const char* source);
void initializeEnemyBatchVAO();
void updateEnemyBatchVBO(Enemy* enemies, int enemyCount, float cameraOffsetX, float cameraOffsetY, float zoomFactor);
void renderStructurePreview(const PlacementMode* mode, float cameraOffsetX, float cameraOffsetY, float zoomFactor);

#endif // RENDERING_H