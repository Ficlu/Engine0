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
EntityBatchData entityBatchData = {NULL, 0};
TileBatchData tileBatchData = {NULL, 0};
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
"uniform float alpha = 1.0;\n"  // Add this line
"void main() {\n"
"    vec4 texColor = texture(textureAtlas, TexCoord);\n"
"    if(texColor.r == 1.0 && texColor.g == 0.0 && texColor.b == 1.0) {\n"
"        discard;\n"
"    }\n"
"    FragColor = vec4(texColor.rgb, texColor.a * alpha);\n"  // Modified this line
"}\n";

// Add these with your other shader sources at the top of rendering.c
const char* uiVertexShaderSource = 
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"  // Add this to receive tex coords
    "out vec2 TexCoord;\n"  // Add this to pass to fragment shader
    "void main() {\n"
    "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
    "    TexCoord = aTexCoord;\n"  // Pass texture coordinates
    "}\0";

const char* uiFragmentShaderSource = 
    "#version 330 core\n"
    "in vec2 TexCoord;\n"
    "out vec4 FragColor;\n"
    "uniform float fillAmount;\n"
    "uniform float flashIntensity;\n"
    "void main() {\n"
    "    vec4 backgroundColor = vec4(0.2, 0.2, 0.2, 0.8);\n"
    "    vec4 fillColor = vec4(0.2, 0.7, 0.2, 0.8);\n"
    "    vec4 borderColor = vec4(0.8, 0.8, 0.8, 0.9);\n"
    "    vec4 flashColor = vec4(1.0, 1.0, 0.7, 0.8);\n"
    "    \n"
    "    float borderThickness = 0.1;\n"
    "    \n"
    "    bool isBorder = \n"
    "        TexCoord.x < borderThickness ||\n"
    "        TexCoord.x > 1.0 - borderThickness ||\n"
    "        TexCoord.y < borderThickness ||\n"
    "        TexCoord.y > 1.0 - borderThickness;\n"
    "    \n"
    "    vec4 baseColor;\n"
    "    if (isBorder) {\n"
    "        baseColor = borderColor;\n"
    "    } else if (TexCoord.x <= fillAmount) {\n"
    "        baseColor = fillColor;\n"
    "    } else {\n"
    "        baseColor = backgroundColor;\n"
    "    }\n"
    "    \n"
    "    // Blend with flash color based on intensity\n"
    "    FragColor = mix(baseColor, flashColor, flashIntensity);\n"
    "}\0";

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

GLuint createUIShaderProgram() {
    GLuint vertexShader = createShader(GL_VERTEX_SHADER, uiVertexShaderSource);
    GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, uiFragmentShaderSource);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("ERROR::UI::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

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
    
    // Create persistent buffer
    entityBatchData.bufferCapacity = MAX_ENEMIES * 6 * 4;
    entityBatchData.persistentBuffer = malloc(entityBatchData.bufferCapacity * sizeof(float));
    
    if (!entityBatchData.persistentBuffer) {
        fprintf(stderr, "Failed to allocate persistent enemy batch buffer\n");
        return;
    }

    glBindVertexArray(enemyBatchVAO);
    glBindBuffer(GL_ARRAY_BUFFER, enemyBatchVBO);
    glBufferData(GL_ARRAY_BUFFER, entityBatchData.bufferCapacity * sizeof(float), NULL, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


void updateEnemyBatchVBO(Enemy* enemies, int enemyCount, float cameraOffsetX, float cameraOffsetY, float zoomFactor) {
    if (!entityBatchData.persistentBuffer) return;

    int dataIndex = 0;
    float enemyTexX = 1.0f / 3.0f;
    float enemyTexY = 5.0f / 6.0f;
    float enemyTexWidth = 1.0f / 3.0f;
    float enemyTexHeight = 1.0f / 6.0f;

    for (int i = 0; i < enemyCount; i++) {
        float enemyScreenX = (enemies[i].entity.posX - cameraOffsetX) * zoomFactor;
        float enemyScreenY = (enemies[i].entity.posY - cameraOffsetY) * zoomFactor;
        float halfSize = TILE_SIZE * zoomFactor;

        float* buffer = entityBatchData.persistentBuffer;

        // First triangle
        buffer[dataIndex++] = enemyScreenX - halfSize;
        buffer[dataIndex++] = enemyScreenY - halfSize;
        buffer[dataIndex++] = enemyTexX;
        buffer[dataIndex++] = enemyTexY;

        buffer[dataIndex++] = enemyScreenX + halfSize;
        buffer[dataIndex++] = enemyScreenY - halfSize;
        buffer[dataIndex++] = enemyTexX + enemyTexWidth;
        buffer[dataIndex++] = enemyTexY;

        buffer[dataIndex++] = enemyScreenX - halfSize;
        buffer[dataIndex++] = enemyScreenY + halfSize;
        buffer[dataIndex++] = enemyTexX;
        buffer[dataIndex++] = enemyTexY + enemyTexHeight;

        // Second triangle
        buffer[dataIndex++] = enemyScreenX + halfSize;
        buffer[dataIndex++] = enemyScreenY - halfSize;
        buffer[dataIndex++] = enemyTexX + enemyTexWidth;
        buffer[dataIndex++] = enemyTexY;

        buffer[dataIndex++] = enemyScreenX + halfSize;
        buffer[dataIndex++] = enemyScreenY + halfSize;
        buffer[dataIndex++] = enemyTexX + enemyTexWidth;
        buffer[dataIndex++] = enemyTexY + enemyTexHeight;

        buffer[dataIndex++] = enemyScreenX - halfSize;
        buffer[dataIndex++] = enemyScreenY + halfSize;
        buffer[dataIndex++] = enemyTexX;
        buffer[dataIndex++] = enemyTexY + enemyTexHeight;
    }

    glBindBuffer(GL_ARRAY_BUFFER, enemyBatchVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, dataIndex * sizeof(float), entityBatchData.persistentBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void cleanupEntityBatchData() {
    if (entityBatchData.persistentBuffer) {
        free(entityBatchData.persistentBuffer);
        entityBatchData.persistentBuffer = NULL;
    }
    entityBatchData.bufferCapacity = 0;
}


void cleanupTileBatchData() {
    if (tileBatchData.persistentBuffer) {
        free(tileBatchData.persistentBuffer);
        tileBatchData.persistentBuffer = NULL;
    }
    tileBatchData.bufferCapacity = 0;
}

void renderStructurePreview(const PlacementMode* mode, float cameraOffsetX, float cameraOffsetY, float zoomFactor) {
    if (!mode->active) return;

    // Convert grid coordinates to screen coordinates
    float posX, posY;
    WorldToScreenCoords(mode->previewX, mode->previewY, cameraOffsetX, cameraOffsetY, zoomFactor, &posX, &posY);

    // Setup alpha blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float texX, texY;
    if (mode->currentType == STRUCTURE_DOOR) {
        texX = 0.0f / 3.0f;
        texY = 1.0f / 6.0f;
    } else {
        // Wall preview uses standard wall texture
        texX = 1.0f / 3.0f;
        texY = 3.0f / 6.0f;
    }

    float texWidth = 1.0f / 3.0f;
    float texHeight = 1.0f / 6.0f;
    float halfSize = TILE_SIZE * zoomFactor;

    // Draw semi-transparent preview
    float previewVertices[] = {
        posX - halfSize, posY - halfSize, texX, texY + texHeight,
        posX + halfSize, posY - halfSize, texX + texWidth, texY + texHeight,
        posX + halfSize, posY + halfSize, texX + texWidth, texY,
        posX - halfSize, posY + halfSize, texX, texY
    };

    glBindVertexArray(squareVAO);
    glBindBuffer(GL_ARRAY_BUFFER, squareVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(previewVertices), previewVertices);

    // Set transparency
    GLint alphaUniform = glGetUniformLocation(shaderProgram, "alpha");
    glUniform1f(alphaUniform, 0.5f); // 50% transparency

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // Reset alpha
    glUniform1f(alphaUniform, 1.0f);
    glDisable(GL_BLEND);
}