#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
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
void RenderText(SDL_Renderer* renderer, const char* text, int x, int y);

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

TTF_Font* font = NULL;

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
        SDL_Delay(1);
    }

    CleanUp();
    SDL_WaitThread(physicsThread, NULL);
    SDL_WaitThread(renderThread, NULL);
}

void Initialize() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS);
    TTF_Init();

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

    window = SDL_CreateWindow("2D Top-Down RPG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    mainContext = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, mainContext);

    glewExperimental = GL_TRUE;
    glewInit();

    glViewport(0, 0, 800, 600);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    lock = SDL_CreateMutex();
    initSemaphore = SDL_CreateSemaphore(0);

    font = TTF_OpenFont("./utils/Jacquard12-Regular.ttf", 24);
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
        // Handle the error appropriately
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
        SDL_LockMutex(lock);
        
        squareX += squareVelocityX * 0.016f;
        squareY += squareVelocityY * 0.016f;

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

void RenderText(SDL_Renderer* renderer, const char* text, int x, int y) {
    SDL_Color textColor = {255, 255, 255, 255};  // White color
    SDL_Surface* textSurface = TTF_RenderText_Blended(font, text, textColor);
    if (!textSurface) {
        printf("Failed to render text: %s\n", TTF_GetError());
        return;
    }
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) {
        printf("Failed to create texture from rendered text: %s\n", SDL_GetError());
        SDL_FreeSurface(textSurface);
        return;
    }
    SDL_Rect renderQuad = {x, y, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, textTexture, NULL, &renderQuad);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}

int RenderLoop(void* arg) {
    (void)arg;
    
    SDL_GLContext renderContext = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, renderContext);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    GLuint VAO, VBO, EBO, shaderProgram;
    CreateSquare(&VAO, &VBO, &EBO, &shaderProgram);

    GLint offsetUniformLocation = glGetUniformLocation(shaderProgram, "uOffset");

    SDL_SemPost(initSemaphore);

    Uint32 frameStart, frameTime;
    char statsText[64];
    Uint32 frameCount = 0;
    Uint32 lastFpsUpdateTime = SDL_GetTicks();
    float currentFps = 0.0f;

    while (isRunning) {
        frameStart = SDL_GetTicks();
        SDL_LockMutex(lock);

        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glUniform2f(offsetUniformLocation, squareX, squareY);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glUseProgram(0);

        // Update FPS counter
        frameCount++;
        
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastFpsUpdateTime >= 1000) {
            currentFps = frameCount / ((currentTime - lastFpsUpdateTime) / 1000.0f);
            frameCount = 0;
            lastFpsUpdateTime = currentTime;
        }

        snprintf(statsText, sizeof(statsText), "FPS: %.2f", currentFps);
        RenderText(renderer, statsText, 10, 10);

        SDL_RenderPresent(renderer);
        SDL_GL_SwapWindow(window);
        SDL_UnlockMutex(lock);

        frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < (Uint32)TARGET_FRAME_TIME) {
            SDL_Delay(TARGET_FRAME_TIME - frameTime);
        }
    }
    
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    SDL_DestroyRenderer(renderer);
    SDL_GL_DeleteContext(renderContext);
    return 0;
}

void PostProcess() {
    // Post-processing logic here
}

void CleanUp() {
    TTF_CloseFont(font);
    TTF_Quit();
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