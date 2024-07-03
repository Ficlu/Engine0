#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdatomic.h>

// Function declarations
void Initialize();
void HandleInput();
int PhysicsLoop(void* arg);
int RenderLoop(void* arg);
void UpdateGameLogic();
void CleanUp();
void CreateSquare(GLuint* VAO, GLuint* VBO, GLuint* EBO, GLuint* shaderProgram);
void InitFramebuffers();

bool isRunning = true;
SDL_Window* window = NULL;
SDL_GLContext mainContext;
SDL_sem* initSemaphore;

GLuint framebuffers[3];
GLuint framebufferTextures[3];
int currentBufferIndex = 0;

// Game state
atomic_int squareX = ATOMIC_VAR_INIT(0);
atomic_int squareY = ATOMIC_VAR_INIT(0);
atomic_int squareVelocityX = ATOMIC_VAR_INIT(500);  // 0.5 * 1000
atomic_int squareVelocityY = ATOMIC_VAR_INIT(300);  // 0.3 * 1000

const int TARGET_FPS = 200;
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

        // Sleep for a short while to prevent a busy loop, but using a more precise method
        SDL_Delay(1); // minimal delay to yield CPU
    }

    CleanUp();
    SDL_WaitThread(physicsThread, NULL);
    SDL_WaitThread(renderThread, NULL);
}

void Initialize() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); // Enable double buffering
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1); // Enable multisampling
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    SDL_GL_SetSwapInterval(1); // Enable V-Sync

    window = SDL_CreateWindow("2D Top-Down RPG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    mainContext = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, mainContext);

    glewExperimental = GL_TRUE;
    glewInit();

    glViewport(0, 0, 800, 600);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    initSemaphore = SDL_CreateSemaphore(0);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    InitFramebuffers();
}

void InitFramebuffers() {
    glGenFramebuffers(3, framebuffers);
    glGenTextures(3, framebufferTextures);

    for (int i = 0; i < 3; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[i]);

        glBindTexture(GL_TEXTURE_2D, framebufferTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferTextures[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            printf("Error initializing framebuffer %d\n", i);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

    glGenVertexArrays(1, VAO);
    glBindVertexArray(*VAO);

    glGenBuffers(1, VBO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    *shaderProgram = glCreateProgram();
    glAttachShader(*shaderProgram, vertexShader);
    glAttachShader(*shaderProgram, fragmentShader);
    glLinkProgram(*shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
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
        
        int x = atomic_load(&squareX);
        int y = atomic_load(&squareY);
        int vx = atomic_load(&squareVelocityX);
        int vy = atomic_load(&squareVelocityY);

        x += (vx * 16) / 1000;  // 0.016 * 1000
        y += (vy * 16) / 1000;  // 0.016 * 1000

        if (x > 900 || x < -900) vx *= -1;  // 0.9 * 1000
        if (y > 900 || y < -900) vy *= -1;  // 0.9 * 1000

        atomic_store(&squareX, x);
        atomic_store(&squareY, y);
        atomic_store(&squareVelocityX, vx);
        atomic_store(&squareVelocityY, vy);

        int interval = MIN_PHYSICS_INTERVAL - (SDL_GetTicks() - startTime);
        if (interval > 0) {
            SDL_Delay(interval);
        }
    }
    return 0;
}

int RenderLoop(void* arg) {
    (void)arg;
    
    SDL_GLContext renderContext = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, renderContext);

    GLuint VAO, VBO, EBO, shaderProgram;
    CreateSquare(&VAO, &VBO, &EBO, &shaderProgram);

    GLint offsetUniformLocation = glGetUniformLocation(shaderProgram, "uOffset");

    SDL_SemPost(initSemaphore);

    Uint32 frameStart, frameTime;
    Uint32 fpsLastTime = SDL_GetTicks();
    int frameCount = 0;

    while (isRunning) {
        frameStart = SDL_GetTicks();

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[currentBufferIndex]);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glUniform2f(offsetUniformLocation, 
                    (float)atomic_load(&squareX) / 1000.0f, 
                    (float)atomic_load(&squareY) / 1000.0f);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glUseProgram(0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, framebufferTextures[currentBufferIndex]);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 800, 600);
        SDL_GL_SwapWindow(window);

        currentBufferIndex = (currentBufferIndex + 1) % 3;

        frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < (Uint32)TARGET_FRAME_TIME) {
            SDL_Delay(TARGET_FRAME_TIME - frameTime);
        }

        frameCount++;
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - fpsLastTime >= 5000) {  // Every 5 seconds
            float fps = frameCount / ((currentTime - fpsLastTime) / 1000.0f);
            printf("FPS: %.2f\n", fps);
            frameCount = 0;
            fpsLastTime = currentTime;
        }
    }
    
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    SDL_GL_DeleteContext(renderContext);
    return 0;
}

void CleanUp() {
    SDL_GL_DeleteContext(mainContext);
    SDL_DestroyWindow(window);
    SDL_DestroySemaphore(initSemaphore);
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    GameLoop();
    return 0;
}
