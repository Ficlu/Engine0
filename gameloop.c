#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// Function declarations
void Initialize();
void HandleInput();
int PhysicsLoop(void* arg);
int RenderLoop(void* arg);
void UpdateGameLogic();
void PostProcess();
void CleanUp();
void CreateSquare();
void CheckGLError(const char* operation);
void CheckShaderError(GLuint shader, const char* type);

bool isRunning = true;
SDL_Window* window = NULL;
SDL_GLContext mainContext;
SDL_mutex* lock;
SDL_sem* initSemaphore;

// Game state
float squareX = 0.0f;
float squareY = 0.0f;
float squareVelocityX = 0.5f;
float squareVelocityY = 0.3f;

const int TARGET_FPS = 60;
const int TARGET_FRAME_TIME = 1000 / TARGET_FPS;
const int LOGIC_UPDATE_INTERVAL = 16; // ~60 updates per second
const int MIN_PHYSICS_INTERVAL = 8; // ~120 physics updates per second

const char* vertexShaderSource = "#version 330 core\n"
"layout(location = 0) in vec3 aPos;\n"
"uniform vec2 uOffset;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos.x + uOffset.x, aPos.y + uOffset.y, aPos.z, 1.0);\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);\n"
"}\n\0";

void CheckGLError(const char* operation) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        fprintf(stderr, "OpenGL error after %s: 0x%x\n", operation, error);
    }
}

void CheckShaderError(GLuint shader, const char* type) {
    GLint success;
    GLchar infoLog[512];
    if(strcmp(type, "PROGRAM") == 0) {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if(!success) {
            glGetProgramInfoLog(shader, 512, NULL, infoLog);
            fprintf(stderr, "ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
        } else {
            printf("Shader program linked successfully.\n");
        }
    } else {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if(!success) {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            fprintf(stderr, "ERROR::SHADER::%s::COMPILATION_FAILED\n%s\n", type, infoLog);
        } else {
            printf("%s shader compiled successfully.\n", type);
        }
    }
}

void GameLoop() {
    Initialize();

    SDL_Thread* physicsThread = SDL_CreateThread(PhysicsLoop, "PhysicsThread", NULL);
    SDL_Thread* renderThread = SDL_CreateThread(RenderLoop, "RenderThread", NULL);

    Uint32 lastTickTime = SDL_GetTicks();

    while (isRunning) {
        Uint32 currentTime = SDL_GetTicks();
        if ((currentTime - lastTickTime) >= (Uint32)LOGIC_UPDATE_INTERVAL) {
            HandleInput();
            UpdateGameLogic();
            lastTickTime = currentTime;
        }
        PostProcess();
        SDL_Delay(1);
    }

    CleanUp();
    SDL_WaitThread(physicsThread, NULL);
    SDL_WaitThread(renderThread, NULL);
}

void Initialize() {
    printf("Initializing SDL...\n");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0) {
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    printf("Setting GL attributes...\n");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

    printf("Creating window...\n");
    window = SDL_CreateWindow("2D Top-Down RPG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "Could not create window: %s\n", SDL_GetError());
        exit(1);
    }

    printf("Creating OpenGL context...\n");
    mainContext = SDL_GL_CreateContext(window);
    if (!mainContext) {
        fprintf(stderr, "Could not create OpenGL context: %s\n", SDL_GetError());
        exit(1);
    }

    printf("Making context current...\n");
    if (SDL_GL_MakeCurrent(window, mainContext) < 0) {
        fprintf(stderr, "Could not make OpenGL context current: %s\n", SDL_GetError());
        exit(1);
    }

    printf("Initializing GLEW...\n");
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        fprintf(stderr, "Could not initialize GLEW: %s\n", glewGetErrorString(glewError));
        exit(1);
    }

    printf("OpenGL version: %s\n", glGetString(GL_VERSION));

    glViewport(0, 0, 800, 600);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    lock = SDL_CreateMutex();
    if (!lock) {
        fprintf(stderr, "Could not create mutex: %s\n", SDL_GetError());
        exit(1);
    }

    initSemaphore = SDL_CreateSemaphore(0);
    if (!initSemaphore) {
        fprintf(stderr, "Could not create semaphore: %s\n", SDL_GetError());
        exit(1);
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void CreateSquare(GLuint* VAO, GLuint* VBO, GLuint* EBO, GLuint* shaderProgram) {
    float vertices[] = {
         0.1f,  0.1f, 0.0f,
         0.1f, -0.1f, 0.0f,
        -0.1f, -0.1f, 0.0f,
        -0.1f,  0.1f, 0.0f
    };
    unsigned int indices[] = {
        0, 1, 3,
        1, 2, 3
    };

    printf("Generating VAO...\n");
    glGenVertexArrays(1, VAO);
    glBindVertexArray(*VAO);

    printf("Generating VBO...\n");
    glGenBuffers(1, VBO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    printf("Generating EBO...\n");
    glGenBuffers(1, EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    printf("Creating vertex shader...\n");
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    CheckShaderError(vertexShader, "VERTEX");

    printf("Creating fragment shader...\n");
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    CheckShaderError(fragmentShader, "FRAGMENT");

    printf("Creating shader program...\n");
    *shaderProgram = glCreateProgram();
    glAttachShader(*shaderProgram, vertexShader);
    glAttachShader(*shaderProgram, fragmentShader);
    glLinkProgram(*shaderProgram);
    CheckShaderError(*shaderProgram, "PROGRAM");

    printf("Validating shader program...\n");
    GLint validProgram;
    glValidateProgram(*shaderProgram);
    glGetProgramiv(*shaderProgram, GL_VALIDATE_STATUS, &validProgram);
    if(validProgram == GL_FALSE) {
        GLchar infoLog[512];
        glGetProgramInfoLog(*shaderProgram, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::PROGRAM::VALIDATION_FAILED\n%s\n", infoLog);
    } else {
        printf("Shader program validated successfully.\n");
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    printf("Setting up vertex attributes...\n");
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    printf("Trying to use shader program...\n");
    glUseProgram(*shaderProgram);
    CheckGLError("glUseProgram in CreateSquare");
    glUseProgram(0);
}

void HandleInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) isRunning = false;
    }
}

void UpdateGameLogic() {
    // Update game logic here
}

int PhysicsLoop(void* arg) {
    (void)arg;
    while (isRunning) {
        Uint32 startTime = SDL_GetTicks();
        SDL_LockMutex(lock);
        
        // Update physics here
        squareX += squareVelocityX * 0.016f; // Assume 16ms frame time
        squareY += squareVelocityY * 0.016f;

        // Simple boundary checking
        if (squareX > 0.9f || squareX < -0.9f) squareVelocityX *= -1;
        if (squareY > 0.9f || squareY < -0.9f) squareVelocityY *= -1;

        SDL_UnlockMutex(lock);

        int interval = MIN_PHYSICS_INTERVAL - (SDL_GetTicks() - startTime);
        if (interval > 0) {
            SDL_Delay(interval);
        }
    }
    return 0;
}

int RenderLoop(void* arg) {
    (void)arg;
    printf("Render thread starting...\n");
    
    SDL_GLContext renderContext = SDL_GL_CreateContext(window);
    if (!renderContext) {
        fprintf(stderr, "Could not create render context: %s\n", SDL_GetError());
        return 1;
    }
    
    if (SDL_GL_MakeCurrent(window, renderContext) < 0) {
        fprintf(stderr, "Could not make render context current: %s\n", SDL_GetError());
        return 1;
    }

    // Initialize GLEW in this thread
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        fprintf(stderr, "Could not initialize GLEW in render thread: %s\n", glewGetErrorString(glewError));
        return 1;
    }

    GLuint VAO, VBO, EBO, shaderProgram;
    CreateSquare(&VAO, &VBO, &EBO, &shaderProgram);

    // Get the location of the uniform
    GLint offsetUniformLocation = glGetUniformLocation(shaderProgram, "uOffset");
    if (offsetUniformLocation == -1) {
        fprintf(stderr, "Could not find uniform 'uOffset'\n");
    }

    // Signal that initialization is complete
    SDL_SemPost(initSemaphore);

    while (isRunning) {
        Uint32 startTime = SDL_GetTicks();
        SDL_LockMutex(lock);

        glClear(GL_COLOR_BUFFER_BIT);

        if (!glIsProgram(shaderProgram)) {
            fprintf(stderr, "Shader program is not valid in RenderLoop\n");
        } else {
            glUseProgram(shaderProgram);
            CheckGLError("glUseProgram in RenderLoop");

            // Set the uniform
            glUniform2f(offsetUniformLocation, squareX, squareY);
            CheckGLError("glUniform2f");

            glBindVertexArray(VAO);
            CheckGLError("glBindVertexArray");

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            CheckGLError("glDrawElements");

            glBindVertexArray(0);
            glUseProgram(0);
        }

        SDL_GL_SwapWindow(window);
        SDL_UnlockMutex(lock);

        int sleepDuration = TARGET_FRAME_TIME - (SDL_GetTicks() - startTime);
        if (sleepDuration > 0) {
            SDL_Delay(sleepDuration);
        }
    }
    
    // Clean up OpenGL resources
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    SDL_GL_DeleteContext(renderContext);
    return 0;
}

void PostProcess() {
    // Post-processing logic here
}

void CleanUp() {
    SDL_GL_DeleteContext(mainContext);
    SDL_DestroyWindow(window);
    SDL_DestroyMutex(lock);
    SDL_DestroySemaphore(initSemaphore);
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    GameLoop();
    return 0;
}