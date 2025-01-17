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
#include "texture_coords.h"

// Global variables
GLuint textureAtlas = 0;
GLuint textureUniform;
GLuint enemyBatchVBO = 0;
GLuint enemyBatchVAO = 0;
EntityBatchData entityBatchData = {NULL, 0};
TileBatchData tileBatchData = {NULL, 0};
GLuint itemShaderProgram = 0;
Viewport gameViewport;
Viewport sidebarViewport;

// UI-specific static globals
static GLuint uiShaderProgram = 0;
static GLuint uiVAO = 0;
static GLuint uiVBO = 0;

// UI resource getters
GLuint getUIShaderProgram(void) { return uiShaderProgram; }
GLuint getUIVAO(void) { return uiVAO; }
GLuint getUIVBO(void) { return uiVBO; }

/**
 * @brief Vertex shader for rendering grid elements.
 *
 * This shader positions grid vertices on screen and forwards texture coordinates.
 */
const char* vertexShaderSource = "#version 330 core\n"
"layout(location = 0) in vec2 position;\n"
"layout(location = 1) in vec2 texCoord;\n"
"out vec2 TexCoord;\n"
"void main() {\n"
"    gl_Position = vec4(position, 0.0, 1.0);\n"
"    TexCoord = texCoord;\n"
"}\n";

/**
 * @brief Fragment shader for rendering textured grid elements.
 *
 * Handles alpha blending and texture sampling for grid elements.
 */
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

/**
 * @brief Vertex shader for rendering UI elements.
 *
 * Processes UI positions, colors, and texture coordinates.
 */
const char* uiVertexShaderSource = 
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "layout (location = 2) in vec4 aColor;\n"
    "out vec2 TexCoord;\n"
    "out vec4 Color;\n"
    "void main() {\n"
    "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
    "    TexCoord = aTexCoord;\n"
    "    Color = aColor;\n"
    "}\0";

/**
 * @brief Fragment shader for UI elements with optional texture and color blending.
 *
 * Supports rendering with or without textures, applying transparency and coloring.
 */
const char* uiFragmentShaderSource = 
    "#version 330 core\n"
    "in vec2 TexCoord;\n"
    "in vec4 Color;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D textureAtlas;\n"
    "uniform bool uHasTexture;\n"
    "void main() {\n"
    "    if (uHasTexture) {\n"
    "        vec4 texColor = texture(textureAtlas, TexCoord);\n"
    "        if (texColor.r == 1.0 && texColor.g == 0.0 && texColor.b == 1.0) {\n"
    "            discard;\n"
    "        }\n"
    "        FragColor = texColor * Color;\n"  // Apply vertex color modulation
    "    } else {\n"
    "        FragColor = Color;\n"
    "    }\n"
    "}\0";


/**
 * @brief Vertex shader for rendering simple outlines.
 *
 * Sets positions for outline vertices.
 */
const char* outlineVertexShaderSource = "#version 330 core\n"
"layout(location = 0) in vec2 position;\n"
"void main() {\n"
"    gl_Position = vec4(position, 0.0, 1.0);\n"
"}\n";

/**
 * @brief Fragment shader for rendering solid color outlines.
 *
 * Outputs a user-defined outline color.
 */
const char* outlineFragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"uniform vec3 outlineColor;\n"
"void main() {\n"
"    FragColor = vec4(outlineColor, 1.0);\n"
"}\n";

/**
 * @brief Vertex shader for rendering item elements.
 *
 * Transforms item vertices and forwards texture coordinates.
 */
const char* itemVertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec2 aPos;\n"
"layout (location = 1) in vec2 aTexCoord;\n"
"out vec2 TexCoord;\n"
"void main() {\n"
"    gl_Position = vec4(aPos, 0.0, 1.0);\n"
"    TexCoord = aTexCoord;\n"
"}\0";

/**
 * @brief Fragment shader for rendering textured items.
 *
 * Samples textures and applies optional color blending for items.
 */
const char* itemFragmentShaderSource = "#version 330 core\n"
"in vec2 TexCoord;\n"
"out vec4 FragColor;\n"
"uniform sampler2D textureAtlas;\n"
"uniform vec4 uColor;\n"
"uniform bool uHasTexture;\n"
"void main() {\n"
"    if (uHasTexture) {\n"
"        vec4 texColor = texture(textureAtlas, TexCoord);\n"
"        if(texColor.a < 0.1) discard;\n"
"        FragColor = texColor;\n"
"    } else {\n"
"        FragColor = uColor;\n"
"    }\n"
"}\n";

void initUIResources(void) {
    uiShaderProgram = createUIShaderProgram();
    
    glGenVertexArrays(1, &uiVAO);
    glGenBuffers(1, &uiVBO);

    glBindVertexArray(uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
    glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

/**
 * @brief Renders a sidebar button with specified dimensions and appearance.
 * 
 * @param x The X-coordinate of the button's top-left corner.
 * @param y The Y-coordinate of the button's top-left corner.
 * @param width The width of the button.
 * @param height The height of the button.
 * 
 * Draws the button's background as a filled rectangle and its border as a line loop.
 */
void renderSidebarButton(float x, float y, float width, float height) {
    float vertices[] = {
        x,        y,
        x+width,  y,
        x+width,  y+height,
        x,        y+height
    };

    glBindVertexArray(uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    
    // Draw button background
    GLint colorLoc = glGetUniformLocation(uiShaderProgram, "color");
    glUniform4f(colorLoc, 0.4f, 0.4f, 0.4f, 1.0f);  // Gray background
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    // Draw button border
    glUniform4f(colorLoc, 0.8f, 0.8f, 0.8f, 1.0f);  // Light gray border
    glDrawArrays(GL_LINE_LOOP, 0, 4);
}

/**
 * @brief Initializes viewports for the game and sidebar.
 * 
 * Sets up the dimensions and positions of the game and sidebar viewports.
 */
void initializeViewports(void) {
    // Initialize game viewport
    gameViewport.x = 0;
    gameViewport.y = 0;
    gameViewport.width = GAME_VIEW_WIDTH;
    gameViewport.height = WINDOW_HEIGHT;

    // Initialize sidebar viewport
    sidebarViewport.x = GAME_VIEW_WIDTH;
    sidebarViewport.y = 0;
    sidebarViewport.width = SIDEBAR_WIDTH;
    sidebarViewport.height = WINDOW_HEIGHT;
}

/**
 * @brief Checks if a point is within the game viewport.
 * 
 * @param x The X-coordinate of the point.
 * @param y The Y-coordinate of the point.
 * @return `true` if the point is inside the game viewport, `false` otherwise.
 */
bool isPointInGameView(int x, int y) {
    return x >= gameViewport.x && 
           x < gameViewport.x + gameViewport.width && 
           y >= gameViewport.y && 
           y < gameViewport.y + gameViewport.height;
}

/**
 * @brief Checks if a point is within the sidebar viewport.
 * 
 * @param x The X-coordinate of the point.
 * @param y The Y-coordinate of the point.
 * @return `true` if the point is inside the sidebar viewport, `false` otherwise.
 */
bool isPointInSidebar(int x, int y) {
    return x >= sidebarViewport.x && 
           x < sidebarViewport.x + sidebarViewport.width && 
           y >= sidebarViewport.y && 
           y < sidebarViewport.y + sidebarViewport.height;
}

/**
 * @brief Applies a specified viewport for rendering.
 * 
 * @param viewport A pointer to the viewport to apply.
 * 
 * Configures the OpenGL viewport based on the specified dimensions and position.
 */
void applyViewport(const Viewport* viewport) {
    glViewport(viewport->x, viewport->y, viewport->width, viewport->height);
}

/**
 * @brief Initializes rendering subsystems, including SDL and OpenGL.
 * 
 * Sets up SDL, creates a window, and initializes GLEW for OpenGL extensions. 
 * Terminates the program on failure.
 */
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

/**
 * @brief Creates a shader program for rendering items.
 *
 * @return The compiled and linked shader program object.
 */
GLuint createItemShaderProgram() {
    GLuint vertexShader = createShader(GL_VERTEX_SHADER, itemVertexShaderSource);
    GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, itemFragmentShaderSource);
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

/**
 * @brief Creates a shader program for rendering UI elements.
 *
 * @return The compiled and linked shader program object.
 */
GLuint createUIShaderProgram() {
    // Add debug prints for shader source
    printf("UI Vertex Shader Source:\n%s\n", uiVertexShaderSource);
    printf("UI Fragment Shader Source:\n%s\n", uiFragmentShaderSource);

    GLuint vertexShader = createShader(GL_VERTEX_SHADER, uiVertexShaderSource);
    GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, uiFragmentShaderSource);

    // Add error checking for shader compilation
    GLint success;
    char infoLog[512];
    
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        printf("ERROR::VERTEX::SHADER::COMPILATION_FAILED\n%s\n", infoLog);
    }

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        printf("ERROR::FRAGMENT::SHADER::COMPILATION_FAILED\n%s\n", infoLog);
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("ERROR::UI::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
    } else {
        // Add validation check
        glValidateProgram(shaderProgram);
        glGetProgramiv(shaderProgram, GL_VALIDATE_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
            printf("ERROR::UI::PROGRAM::VALIDATION_FAILED\n%s\n", infoLog);
        }
    }

    // After linking, verify uniform locations
    printf("Checking UI shader uniforms:\n");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "uColor");
    GLint borderColorLoc = glGetUniformLocation(shaderProgram, "uBorderColor");
    GLint borderWidthLoc = glGetUniformLocation(shaderProgram, "uBorderWidth");
    GLint hasTextureLoc = glGetUniformLocation(shaderProgram, "uHasTexture");
    GLint hasBorderLoc = glGetUniformLocation(shaderProgram, "uHasBorder");
    
    printf("uColor location: %d\n", colorLoc);
    printf("uBorderColor location: %d\n", borderColorLoc);
    printf("uBorderWidth location: %d\n", borderWidthLoc);
    printf("uHasTexture location: %d\n", hasTextureLoc);
    printf("uHasBorder location: %d\n", hasBorderLoc);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

/**
 * @brief Creates a shader program for rendering outlines.
 *
 * @return The compiled and linked shader program object.
 */
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

/**
 * @brief Initializes a VAO for rendering outlines.
 */
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

/**
 * @brief Compiles a shader from the provided source code.
 *
 * @param type The type of shader (e.g., `GL_VERTEX_SHADER` or `GL_FRAGMENT_SHADER`).
 * @param source The source code of the shader.
 * @return The compiled shader object.
 */
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

/**
 * @brief Links a vertex and fragment shader into a shader program.
 *
 * @return The linked shader program object.
 */
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

/**
 * @brief Generates vertex data for a grid.
 *
 * @param vertices Pointer to the array of generated vertex data.
 * @param vertexCount Pointer to the vertex count.
 * @param width, height Dimensions of the grid area.
 * @param cellSize The size of each grid cell.
 */
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

/**
 * @brief Creates a vertex array object (VAO) for a grid.
 * 
 * @param vertices An array of vertex data for the grid.
 * @param vertexCount The number of vertices in the grid.
 * @return The generated VAO for the grid.
 */
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

/**
 * @brief Creates a vertex array object (VAO) for a square with texture coordinates.
 * 
 * @param size The size of the square.
 * @param texX, texY The top-left texture coordinates.
 * @param texWidth, texHeight The width and height of the texture region.
 * @return The generated VAO for the square.
 */
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

/**
 * @brief Loads a BMP texture and uploads it to the GPU.
 * 
 * @param filePath The file path to the BMP texture.
 * @return The OpenGL texture ID for the loaded texture.
 * 
 * Reads BMP file data, creates a texture, and sets texture parameters.
 */
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

/**
 * @brief Initializes the vertex array object (VAO) and buffer for enemy batches.
 * 
 * Sets up persistent buffer memory for batching enemy rendering data.
 */
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

/**
 * @brief Updates the buffer data for rendering a batch of enemies.
 * 
 * @param enemies An array of enemy entities to render.
 * @param enemyCount The number of enemies in the array.
 * @param cameraOffsetX, cameraOffsetY The camera's X and Y offsets.
 * @param zoomFactor The zoom factor for rendering.
 * 
 * Populates the buffer with transformed vertex data for each enemy.
 */
void updateEnemyBatchVBO(Enemy* enemies, int enemyCount, float cameraOffsetX, float cameraOffsetY, float zoomFactor) {
    if (enemyCount == 0) return;

    TextureCoords* enemyTex = getTextureCoords("enemy");
    if (!enemyTex) {
        fprintf(stderr, "Failed to get enemy texture coordinates\n");
        return;
    }

    float* vertices = (float*)malloc(enemyCount * 24 * sizeof(float));  // 6 vertices * 4 components
    if (!vertices) {
        fprintf(stderr, "Failed to allocate vertex buffer for enemies\n");
        return;
    }

    int vertexIndex = 0;
    for (int i = 0; i < enemyCount; i++) {
        float enemyScreenX = (enemies[i].entity.posX - cameraOffsetX) * zoomFactor;
        float enemyScreenY = (enemies[i].entity.posY - cameraOffsetY) * zoomFactor;
        
        // First triangle
        vertices[vertexIndex++] = enemyScreenX - TILE_SIZE * zoomFactor;
        vertices[vertexIndex++] = enemyScreenY - TILE_SIZE * zoomFactor;
        vertices[vertexIndex++] = enemyTex->u1;
        vertices[vertexIndex++] = enemyTex->v1;

        vertices[vertexIndex++] = enemyScreenX + TILE_SIZE * zoomFactor;
        vertices[vertexIndex++] = enemyScreenY - TILE_SIZE * zoomFactor;
        vertices[vertexIndex++] = enemyTex->u2;
        vertices[vertexIndex++] = enemyTex->v1;

        vertices[vertexIndex++] = enemyScreenX + TILE_SIZE * zoomFactor;
        vertices[vertexIndex++] = enemyScreenY + TILE_SIZE * zoomFactor;
        vertices[vertexIndex++] = enemyTex->u2;
        vertices[vertexIndex++] = enemyTex->v2;

        // Second triangle
        vertices[vertexIndex++] = enemyScreenX - TILE_SIZE * zoomFactor;
        vertices[vertexIndex++] = enemyScreenY - TILE_SIZE * zoomFactor;
        vertices[vertexIndex++] = enemyTex->u1;
        vertices[vertexIndex++] = enemyTex->v1;

        vertices[vertexIndex++] = enemyScreenX + TILE_SIZE * zoomFactor;
        vertices[vertexIndex++] = enemyScreenY + TILE_SIZE * zoomFactor;
        vertices[vertexIndex++] = enemyTex->u2;
        vertices[vertexIndex++] = enemyTex->v2;

        vertices[vertexIndex++] = enemyScreenX - TILE_SIZE * zoomFactor;
        vertices[vertexIndex++] = enemyScreenY + TILE_SIZE * zoomFactor;
        vertices[vertexIndex++] = enemyTex->u1;
        vertices[vertexIndex++] = enemyTex->v2;
    }

    glBindBuffer(GL_ARRAY_BUFFER, enemyBatchVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexIndex * sizeof(float), vertices);
    
    free(vertices);
}
/**
 * @brief Frees resources associated with the entity batch data.
 * 
 * Releases the memory used by the persistent buffer and resets capacity.
 */
void cleanupEntityBatchData() {
    if (entityBatchData.persistentBuffer) {
        free(entityBatchData.persistentBuffer);
        entityBatchData.persistentBuffer = NULL;
    }
    entityBatchData.bufferCapacity = 0;
}

/**
 * @brief Frees resources associated with the tile batch data.
 * 
 * Releases the memory used by the persistent buffer and resets capacity.
 */
void cleanupTileBatchData() {
    if (tileBatchData.persistentBuffer) {
        free(tileBatchData.persistentBuffer);
        tileBatchData.persistentBuffer = NULL;
    }
    tileBatchData.bufferCapacity = 0;
}

/**
 * @brief Configures the OpenGL viewport for the game area.
 * 
 * Sets the viewport to cover the game view area.
 */
void setupGameViewport(void) {
    glViewport(0, 0, GAME_VIEW_WIDTH, WINDOW_HEIGHT);
}

/**
 * @brief Configures the OpenGL viewport for the sidebar.
 * 
 * Sets the viewport to cover the sidebar area.
 */
void setupSidebarViewport(void) {
    glViewport(GAME_VIEW_WIDTH, 0, SIDEBAR_WIDTH, WINDOW_HEIGHT);
}

/**
 * @brief Renders a preview of a structure being placed.
 * 
 * @param mode The placement mode containing preview details.
 * @param cameraOffsetX, cameraOffsetY The camera's X and Y offsets.
 * @param zoomFactor The zoom factor for rendering.
 * 
 * Draws a semi-transparent preview of the structure at the specified grid coordinates.
 */
void renderStructurePreview(const PlacementMode* mode, float cameraOffsetX, float cameraOffsetY, float zoomFactor) {
    if (!mode->active) return;

    float posX, posY;
    WorldToScreenCoords(mode->previewX, mode->previewY, cameraOffsetX, cameraOffsetY, zoomFactor, &posX, &posY);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float texX, texY;
    if (mode->currentType == STRUCTURE_DOOR) {
        texX = 0.0f / 3.0f;
        texY = 1.0f / 6.0f;
    } else {
        texX = 1.0f / 3.0f;
        texY = 3.0f / 6.0f;
    }

    float texWidth = 1.0f / 3.0f;
    float texHeight = 1.0f / 6.0f;
    float halfSize = TILE_SIZE * zoomFactor;

float previewVertices[] = {
    posX - halfSize, (posY - halfSize) + halfSize, texX, texY + texHeight,
    posX + halfSize, (posY - halfSize) + halfSize, texX + texWidth, texY + texHeight,
    posX + halfSize, posY + halfSize + halfSize, texX + texWidth, texY,
    posX - halfSize, posY + halfSize + halfSize, texX, texY
};
    glBindVertexArray(squareVAO);
    glBindBuffer(GL_ARRAY_BUFFER, squareVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(previewVertices), previewVertices);

    GLint alphaUniform = glGetUniformLocation(shaderProgram, "alpha");
    glUniform1f(alphaUniform, 0.5f);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glUniform1f(alphaUniform, 1.0f);
    glDisable(GL_BLEND);
}
void cleanupUIResources(void) {
    if (uiShaderProgram) {
        glDeleteProgram(uiShaderProgram);
        uiShaderProgram = 0;
    }
    if (uiVAO) {
        glDeleteVertexArrays(1, &uiVAO);
        uiVAO = 0;
    }
    if (uiVBO) {
        glDeleteBuffers(1, &uiVBO);
        uiVBO = 0;
    }
}