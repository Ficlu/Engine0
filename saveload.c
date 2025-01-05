// saveload.c
#include "saveload.h"
#include "player.h"
#include "grid.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdatomic.h>


extern Player player;

bool saveGameState(const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        printf("Failed to open save file for writing: %s\n", filename);
        return false;
    }

    // Write header
    fwrite(MAGIC_NUMBER, 1, 4, file);
    uint32_t version = SAVE_VERSION;
    fwrite(&version, sizeof(uint32_t), 1, file);
    uint32_t timestamp = (uint32_t)time(NULL);
    fwrite(&timestamp, sizeof(uint32_t), 1, file);

    // Save player position
    int32_t gridX = atomic_load(&player.entity.gridX);
    int32_t gridY = atomic_load(&player.entity.gridY);
    float posX = atomic_load(&player.entity.posX);
    float posY = atomic_load(&player.entity.posY);
    
    fwrite(&gridX, sizeof(int32_t), 1, file);
    fwrite(&gridY, sizeof(int32_t), 1, file);
    fwrite(&posX, sizeof(float), 1, file);
    fwrite(&posY, sizeof(float), 1, file);

    // Count and save structures (walls/doors)
    uint32_t structureCount = 0;
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (grid[y][x].hasWall) {
                structureCount++;
            }
        }
    }

    fwrite(&structureCount, sizeof(uint32_t), 1, file);

    // Write each structure's complete data
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (grid[y][x].hasWall) {
                uint16_t structX = (uint16_t)x;
                uint16_t structY = (uint16_t)y;
                uint8_t flags = 0;
                flags |= grid[y][x].isWalkable ? 1 : 0;
                flags |= grid[y][x].isDoorOpen ? 2 : 0;
                float texX = grid[y][x].wallTexX;
                float texY = grid[y][x].wallTexY;

                fwrite(&structX, sizeof(uint16_t), 1, file);
                fwrite(&structY, sizeof(uint16_t), 1, file);
                fwrite(&flags, sizeof(uint8_t), 1, file);
                fwrite(&texX, sizeof(float), 1, file);
                fwrite(&texY, sizeof(float), 1, file);
            }
        }
    }

    fclose(file);
    printf("Game saved successfully to: %s\n", filename);
    return true;
}

bool loadGameState(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Failed to open save file for reading: %s\n", filename);
        return false;
    }

    // Verify header
    char magic[5] = {0};
    fread(magic, 1, 4, file);
    if (strcmp(magic, MAGIC_NUMBER) != 0) {
        printf("Invalid save file format\n");
        fclose(file);
        return false;
    }

    uint32_t version;
    fread(&version, sizeof(uint32_t), 1, file);
    if (version != SAVE_VERSION) {
        printf("Unsupported save file version: %u\n", version);
        fclose(file);
        return false;
    }

    // Skip timestamp
    uint32_t timestamp;
    fread(&timestamp, sizeof(uint32_t), 1, file);

    // Load player position
    int32_t gridX, gridY;
    float posX, posY;
    
    fread(&gridX, sizeof(int32_t), 1, file);
    fread(&gridY, sizeof(int32_t), 1, file);
    fread(&posX, sizeof(float), 1, file);
    fread(&posY, sizeof(float), 1, file);

    // Update player position
    atomic_store(&player.entity.gridX, gridX);
    atomic_store(&player.entity.gridY, gridY);
    atomic_store(&player.entity.posX, posX);
    atomic_store(&player.entity.posY, posY);

    // Reset camera to player position
    atomic_store(&player.cameraTargetX, posX);
    atomic_store(&player.cameraTargetY, posY);
    atomic_store(&player.cameraCurrentX, posX);
    atomic_store(&player.cameraCurrentY, posY);

    // Clear any active movement/pathfinding
    atomic_store(&player.entity.targetGridX, gridX);  // Reset target to current position
    atomic_store(&player.entity.targetGridY, gridY);
    atomic_store(&player.entity.finalGoalX, gridX);   // Reset final goal to current position
    atomic_store(&player.entity.finalGoalY, gridY);
    atomic_store(&player.entity.needsPathfinding, false);  // Turn off pathfinding

    // Reset any pending build targets
    player.hasBuildTarget = false;
    player.targetBuildX = 0;
    player.targetBuildY = 0;
    // Clear all existing structures
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            grid[y][x].hasWall = false;
            grid[y][x].isWalkable = true;
            grid[y][x].isDoorOpen = false;
        }
    }

    // Load structures
    uint32_t structureCount;
    fread(&structureCount, sizeof(uint32_t), 1, file);

    for (uint32_t i = 0; i < structureCount; i++) {
        uint16_t structX, structY;
        uint8_t flags;
        float texX, texY;

        fread(&structX, sizeof(uint16_t), 1, file);
        fread(&structY, sizeof(uint16_t), 1, file);
        fread(&flags, sizeof(uint8_t), 1, file);
        fread(&texX, sizeof(float), 1, file);
        fread(&texY, sizeof(float), 1, file);

        if (structX < GRID_SIZE && structY < GRID_SIZE) {
            grid[structY][structX].hasWall = true;
            grid[structY][structX].isWalkable = (flags & 1) != 0;
            grid[structY][structX].isDoorOpen = (flags & 2) != 0;
            grid[structY][structX].wallTexX = texX;
            grid[structY][structX].wallTexY = texY;
        }
    }

    fclose(file);
    printf("Game loaded successfully from: %s\n", filename);
    return true;
}