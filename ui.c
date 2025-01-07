#include "ui.h"
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <math.h>

static GLuint uiVAO;
static GLuint uiVBO;
static GLuint uiShaderProgram;
static float expGainFlashIntensity = 0.0f;
static Uint32 lastExpGainTime = 0;
static float lastKnownExp = 0.0f;

void InitializeUI(GLuint shaderProgram) {
    uiShaderProgram = shaderProgram;
    
    glGenVertexArrays(1, &uiVAO);
    glGenBuffers(1, &uiVBO);
    
    // Adjust these values to change position and size
    const float vertices[] = {
        // Positions           // TexCoords
        0.70f,  0.80f,        0.0f, 0.0f,  // Bottom left
        0.95f,  0.80f,        1.0f, 0.0f,  // Bottom right
        0.95f,  0.85f,        1.0f, 1.0f,  // Top right
        0.70f,  0.85f,        0.0f, 1.0f   // Top left
    };

    glBindVertexArray(uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    printf("UI initialized with shader program %u\n", shaderProgram);
}
void RenderUI(const Player* player, GLuint shaderProgram) {
    glUseProgram(shaderProgram);

    float totalExp = player->skills.constructionExp;
    int currentLevel = (int)(totalExp / EXP_PER_LEVEL);
    
    // Check for exp gain
    if (totalExp > lastKnownExp) {
        expGainFlashIntensity = 1.0f;
        lastExpGainTime = SDL_GetTicks();
        lastKnownExp = totalExp;
    }
    
    // Update flash effect
    Uint32 currentTime = SDL_GetTicks();
    if (expGainFlashIntensity > 0.0f) {
        Uint32 timeSinceGain = currentTime - lastExpGainTime;
        if (timeSinceGain < FLASH_DURATION_MS) {
            expGainFlashIntensity = 1.0f - ((float)timeSinceGain / FLASH_DURATION_MS);
        } else {
            expGainFlashIntensity = 0.0f;
        }
    }

    float expProgress = fmodf(totalExp, EXP_PER_LEVEL);
    float fillAmount = expProgress / EXP_PER_LEVEL;
    
    // Set uniforms
    GLint fillLoc = glGetUniformLocation(shaderProgram, "fillAmount");
    glUniform1f(fillLoc, fillAmount);
    
    GLint flashLoc = glGetUniformLocation(shaderProgram, "flashIntensity");
    glUniform1f(flashLoc, expGainFlashIntensity);

    printf("Level %d - Progress: %.1f%%\n", currentLevel, fillAmount * 100.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glBindVertexArray(uiVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisable(GL_BLEND);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void CleanupUI(void) {
    glDeleteVertexArrays(1, &uiVAO);
    glDeleteBuffers(1, &uiVBO);
}