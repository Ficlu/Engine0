#include "rendering.h"
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

GLuint textureAtlas;
GLuint textureUniform;

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
"    FragColor = texture(textureAtlas, TexCoord);\n"
"}\n";

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
    int size = (width < height) ? width : height;  // Use the smaller dimension
    int numCells = size / cellSize;
    int numLines = (numCells + 1) * 2;  // Vertical + Horizontal lines
    *vertexCount = numLines * 2;  // Each line has 2 points
    *vertices = (float*)malloc(*vertexCount * 2 * sizeof(float));  // 2 floats per point (x, y)

    int index = 0;
    float step = 2.0f / numCells;  // Step size in NDC

    // Create vertical lines
    for (int i = 0; i <= numCells; i++) {
        float x = i * step - 1.0f;
        (*vertices)[index++] = x;
        (*vertices)[index++] = -1.0f;
        (*vertices)[index++] = x;
        (*vertices)[index++] = 1.0f;
    }

    // Create horizontal lines
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
    float halfSize = size / 2.0f;
    float vertices[] = {
        // Positions       // Texture Coords
        -halfSize, -halfSize, texX, texY + texHeight,
         halfSize, -halfSize, texX + texWidth, texY + texHeight,
         halfSize,  halfSize, texX + texWidth, texY,
        -halfSize,  halfSize, texX, texY
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
    // Open the file
    FILE* file = fopen(filePath, "rb");
    if (!file) {
        printf("Image could not be opened: %s\n", filePath);
        return 0;
    }

    // Read the header
    unsigned char header[54];
    if (fread(header, 1, 54, file) != 54) {
        printf("Not a correct BMP file: %s\n", filePath);
        fclose(file);
        return 0;
    }

    // Parse header information
    unsigned int dataPos = *(int*)&(header[0x0A]);
    unsigned int imageSize = *(int*)&(header[0x22]);
    unsigned int width = *(int*)&(header[0x12]);
    unsigned int height = *(int*)&(header[0x16]);

    // Some BMP files are misformatted, guess missing information
    if (imageSize == 0) imageSize = width * height * 3;
    if (dataPos == 0) dataPos = 54;

    // Create buffer
    unsigned char* data = (unsigned char*)malloc(imageSize);

    // Read the actual data from the file into the buffer
    fread(data, 1, imageSize, file);

    // Close the file
    fclose(file);

    // Create one OpenGL texture
    GLuint textureID;
    glGenTextures(1, &textureID);

    // "Bind" the newly created texture
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Give the image to OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);

    free(data);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    printf("Texture loaded successfully: %s (Width: %d, Height: %d)\n", filePath, width, height);
    return textureID;
}