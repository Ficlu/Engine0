#ifndef RENDERING_H
#define RENDERING_H

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <stdlib.h>

// Function declarations
void initRendering();
GLuint createShaderProgram();
void createGridVertices(float** vertices, int* vertexCount, int width, int height, int cellSize);
GLuint createGridVAO(float* vertices, int vertexCount);
GLuint createSquareVAO(float size);

#endif // RENDERING_H
