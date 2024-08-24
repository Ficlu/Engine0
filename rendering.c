// rendering.c

#include "rendering.h"
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/glew.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "gameloop.h"
GLuint textureAtlas = 0;
GLuint textureUniform;
GLuint enemyBatchVBO = 0;
GLuint enemyBatchVAO = 0;

void initRendering() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_Window* window = SDL_CreateWindow("Grid", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_GL_CreateContext(window);
    glewExperimental = GL_TRUE; 
    if (glewInit() != GLEW_OK) {
        printf("Error initializing GLEW\n");
        exit(1);
    }
}

const char* vertexShaderSource = "#version 330 core\n"
"layout(location = 0) in vec2 position;\n"
"layout(location = 1) in vec2 texCoord;\n"
"out vec2 TexCoord;\n"
"void main() {\n"
"    gl_Position = vec4(position, 0.0, 1.0);\n"
"    TexCoord = texCoord;\n"
"}\n";

const char* fragmentShaderSource = "#version 330 core\n"
"in vec2 TexCoord;\n"
"out vec4 FragColor;\n"
"uniform sampler2D textureAtlas;\n"
"void main() {\n"
"    vec4 texColor = texture(textureAtlas, TexCoord);\n"
"    if(texColor.r == 1.0 && texColor.g == 0.0 && texColor.b == 1.0) {\n"
"        discard;\n"
"    }\n"
"    FragColor = texColor;\n"
"}\n";

const char* outlineVertexShaderSource = "#version 330 core\n"
"layout(location = 0) in vec2 position;\n"
"void main() {\n"
"    gl_Position = vec4(position, 0.0, 1.0);\n"
"}\n";

const char* outlineFragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"uniform vec3 outlineColor;\n"
"void main() {\n"
"    FragColor = vec4(outlineColor, 1.0);\n"
"}\n";

GLuint createOutlineShaderProgram() {
    GLuint vertexShader = createShader(GL_VERTEX_SHADER, outlineVertexShaderSource);
    GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, outlineFragmentShaderSource);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("ERROR::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
    } else {
        printf("Outline shader program linked successfully\n");
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

GLuint outlineVAO, outlineVBO;

void initializeOutlineVAO() {
    glGenVertexArrays(1, &outlineVAO);
    glGenBuffers(1, &outlineVBO);

    glBindVertexArray(outlineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, outlineVBO);

    glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), NULL, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    printf("Outline VAO initialized\n");
}

GLuint createShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("ERROR::SHADER::COMPILATION_FAILED\n%s\n", infoLog);
    } else {
        printf("Shader compiled successfully\n");
    }

    return shader;
}

GLuint createShaderProgram() {
    GLuint vertexShader = createShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("ERROR::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
    } else {
        printf("Shader program linked successfully\n");
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void createGridVertices(float** vertices, int* vertexCount, int width, int height, int cellSize) {
    int size = (width < height) ? width : height;
    int numCells = size / cellSize;
    int numLines = (numCells + 1) * 2;
    *vertexCount = numLines * 2;
    *vertices = (float*)malloc(*vertexCount * 2 * sizeof(float));

    int index = 0;
    float step = 2.0f / numCells;

    for (int i = 0; i <= numCells; i++) {
        float x = i * step - 1.0f;
        (*vertices)[index++] = x;
        (*vertices)[index++] = -1.0f;
        (*vertices)[index++] = x;
        (*vertices)[index++] = 1.0f;
    }

    for (int j = 0; j <= numCells; j++) {
        float y = j * step - 1.0f;
        (*vertices)[index++] = -1.0f;
        (*vertices)[index++] = y;
        (*vertices)[index++] = 1.0f;
        (*vertices)[index++] = y;
    }
}

GLuint createGridVAO(float* vertices, int vertexCount) {
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * 2 * sizeof(float), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    printf("Grid VAO created successfully\n");

    return VAO;
}

GLuint createSquareVAO(float size, float texX, float texY, float texWidth, float texHeight) {
    float vertices[] = {
        -size, -size, texX, texY + texHeight,
         size, -size, texX + texWidth, texY + texHeight,
         size,  size, texX + texWidth, texY,
        -size,  size, texX, texY
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}

GLuint loadBMP(const char* filePath) {
    FILE* file = fopen(filePath, "rb");
    if (!file) {
        printf("Image could not be opened: %s\n", filePath);
        return 0;
    }

    unsigned char header[54];
    if (fread(header, 1, 54, file) != 54) {
        printf("Not a correct BMP file: %s\n", filePath);
        fclose(file);
        return 0;
    }

    unsigned int dataPos = *(int*)&(header[0x0A]);
    unsigned int imageSize = *(int*)&(header[0x22]);
    unsigned int width = *(int*)&(header[0x12]);
    unsigned int height = *(int*)&(header[0x16]);

    if (imageSize == 0) imageSize = width * height * 3;
    if (dataPos == 0) dataPos = 54;

    unsigned char* data = (unsigned char*)malloc(imageSize);

    fread(data, 1, imageSize, file);
    fclose(file);

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
    free(data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    printf("Texture loaded successfully: %s (Width: %d, Height: %d)\n", filePath, width, height);
    return textureID;
}

void initializeEnemyBatchVAO() {
    glGenVertexArrays(1, &enemyBatchVAO);
    glGenBuffers(1, &enemyBatchVBO);

    glBindVertexArray(enemyBatchVAO);
    glBindBuffer(GL_ARRAY_BUFFER, enemyBatchVBO);

    glBufferData(GL_ARRAY_BUFFER, MAX_ENEMIES * 6 * 4 * sizeof(float), NULL, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


void updateEnemyBatchVBO(Enemy* enemies, int enemyCount, float cameraOffsetX, float cameraOffsetY, float zoomFactor) {
    float* batchData = (float*)malloc(enemyCount * 6 * 4 * sizeof(float));
    if (!batchData) {
        fprintf(stderr, "Failed to allocate memory for enemy batch data\n");
        return;
    }

    int dataIndex = 0;
    float enemyTexX = 1.0f / 3.0f;
    float enemyTexY = 1.0f / 2.0f;
    float enemyTexWidth = 1.0f / 3.0f;
    float enemyTexHeight = 1.0f / 2.0f;

    for (int i = 0; i < enemyCount; i++) {
        float enemyScreenX = (enemies[i].entity.posX - cameraOffsetX) * zoomFactor;
        float enemyScreenY = (enemies[i].entity.posY - cameraOffsetY) * zoomFactor;
        float halfSize = TILE_SIZE * zoomFactor;

        // First triangle
        // Top-left vertex
        batchData[dataIndex++] = enemyScreenX - halfSize;
        batchData[dataIndex++] = enemyScreenY - halfSize;
        batchData[dataIndex++] = enemyTexX;
        batchData[dataIndex++] = enemyTexY;

        // Top-right vertex
        batchData[dataIndex++] = enemyScreenX + halfSize;
        batchData[dataIndex++] = enemyScreenY - halfSize;
        batchData[dataIndex++] = enemyTexX + enemyTexWidth;
        batchData[dataIndex++] = enemyTexY;

        // Bottom-left vertex
        batchData[dataIndex++] = enemyScreenX - halfSize;
        batchData[dataIndex++] = enemyScreenY + halfSize;
        batchData[dataIndex++] = enemyTexX;
        batchData[dataIndex++] = enemyTexY + enemyTexHeight;

        // Second triangle
        // Top-right vertex
        batchData[dataIndex++] = enemyScreenX + halfSize;
        batchData[dataIndex++] = enemyScreenY - halfSize;
        batchData[dataIndex++] = enemyTexX + enemyTexWidth;
        batchData[dataIndex++] = enemyTexY;

        // Bottom-right vertex
        batchData[dataIndex++] = enemyScreenX + halfSize;
        batchData[dataIndex++] = enemyScreenY + halfSize;
        batchData[dataIndex++] = enemyTexX + enemyTexWidth;
        batchData[dataIndex++] = enemyTexY + enemyTexHeight;

        // Bottom-left vertex
        batchData[dataIndex++] = enemyScreenX - halfSize;
        batchData[dataIndex++] = enemyScreenY + halfSize;
        batchData[dataIndex++] = enemyTexX;
        batchData[dataIndex++] = enemyTexY + enemyTexHeight;
    }

    glBindBuffer(GL_ARRAY_BUFFER, enemyBatchVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, enemyCount * 6 * 4 * sizeof(float), batchData);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    free(batchData);
}