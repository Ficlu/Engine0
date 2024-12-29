#ifndef RENDERING_H
#define RENDERING_H

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <stdlib.h>
#include "enemy.h"
#include "gameloop.h"

typedef struct {
    float* persistentBuffer;
    size_t bufferCapacity;
} EntityBatchData;

typedef struct {
    float* persistentBuffer;
    size_t bufferCapacity;
} TileBatchData;

extern TileBatchData tileBatchData;


extern EntityBatchData entityBatchData;
void cleanupEntityBatchData(void);

void cleanupTileBatchData(void);
void cleanupEntityBatchData(void);

extern GLuint textureAtlas;
extern GLuint textureUniform;
extern GLuint enemyBatchVBO;
extern GLuint enemyBatchVAO;
extern GLuint outlineVBO;
// Function declarations
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

#endif // RENDERING_H