#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <stdio.h>
#include <stdlib.h>

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
"out vec4 vertexColor;\n"
"uniform vec4 color;\n"
"void main() {\n"
"    gl_Position = vec4(position, 0.0, 1.0);\n"
"    vertexColor = color;\n"
"}\n";

const char* fragmentShaderSource = "#version 330 core\n"
"in vec4 vertexColor;\n"
"out vec4 FragColor;\n"
"void main() {\n"
"    FragColor = vertexColor;\n"
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

GLuint createSquareVAO(float size) {
    float halfSize = size / 2.0f;
    float vertices[] = {
        -halfSize, -halfSize,
         halfSize, -halfSize,
         halfSize,  halfSize,
        -halfSize,  halfSize
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    printf("Square VAO created successfully\n");

    return VAO;
}