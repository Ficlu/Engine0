// saveload.c
#include "saveload.h"
#include "player.h"
#include "grid.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdatomic.h>
#include "structures.h"
#include <stdlib.h>
#include "enemy.h" 
#include "entity.h" 
#include "gameloop.h"

extern Player player;
extern Enemy enemies[MAX_ENEMIES];
extern void WorldToScreenCoords(int gridX, int gridY, float cameraOffsetX, float cameraOffsetY, float zoomFactor, float* screenX, float* screenY);

bool saveGameState(const char* filename) {
    printf("[DEBUG] Starting saveGameState\n");
    FILE* file = fopen(filename, "wb");
    if (!file) {
        printf("[ERROR] Failed to open save file for writing: %s\n", filename);
        return false;
    }

    printf("[DEBUG] Writing header\n");
    fwrite(MAGIC_NUMBER, 1, 4, file);
    uint32_t version = SAVE_VERSION;
    fwrite(&version, sizeof(uint32_t), 1, file);
    uint32_t timestamp = (uint32_t)time(NULL);
    fwrite(&timestamp, sizeof(uint32_t), 1, file);

    printf("[DEBUG] Saving player position\n");
    int32_t gridX = atomic_load(&player.entity.gridX);
    int32_t gridY = atomic_load(&player.entity.gridY);
    float posX = atomic_load(&player.entity.posX);
    float posY = atomic_load(&player.entity.posY);
    
    fwrite(&gridX, sizeof(int32_t), 1, file);
    fwrite(&gridY, sizeof(int32_t), 1, file);
    fwrite(&posX, sizeof(float), 1, file);
    fwrite(&posY, sizeof(float), 1, file);

    // Save player skills
    printf("[DEBUG] Saving player skills\n");
    float constructionExp = player.skills.constructionExp;
    fwrite(&constructionExp, sizeof(float), 1, file);

    printf("[DEBUG] Counting and saving structures\n");
    uint32_t structureCount = 0;
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (grid[y][x].hasWall) {
                structureCount++;
            }
        }
    }

    fwrite(&structureCount, sizeof(uint32_t), 1, file);
    printf("[DEBUG] Total structures: %u\n", structureCount);

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

    printf("[DEBUG] Saving enclosures\n");
    uint32_t enclosureCount = (uint32_t)globalEnclosureManager.count;
    fwrite(&enclosureCount, sizeof(uint32_t), 1, file);

    for (int i = 0; i < globalEnclosureManager.count; i++) {
        EnclosureData* enclosure = &globalEnclosureManager.enclosures[i];
        
        fwrite(&enclosure->hash, sizeof(uint64_t), 1, file);
        fwrite(&enclosure->boundaryCount, sizeof(int), 1, file);
        fwrite(&enclosure->interiorCount, sizeof(int), 1, file);
        fwrite(&enclosure->totalArea, sizeof(int), 1, file);
        fwrite(&enclosure->centerPoint, sizeof(Point), 1, file);
        fwrite(&enclosure->doorCount, sizeof(int), 1, file);
        fwrite(&enclosure->wallCount, sizeof(int), 1, file);
        fwrite(&enclosure->isValid, sizeof(bool), 1, file);

        for (int j = 0; j < enclosure->boundaryCount; j++) {
            fwrite(&enclosure->boundaryTiles[j], sizeof(Point), 1, file);
        }

        for (int j = 0; j < enclosure->interiorCount; j++) {
            fwrite(&enclosure->interiorTiles[j], sizeof(Point), 1, file);
        }
    }

    fclose(file);
    printf("[DEBUG] Game saved successfully to: %s\n", filename);
    return true;
}

bool loadGameState(const char* filename) {
    printf("Loading game state from: %s\n", filename);
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Failed to open save file\n");
        return false;
    }

    // 1. Version check
    char magic[5] = {0};
    uint32_t version;
    uint32_t timestamp;
    
    fread(magic, 1, 4, file);
    fread(&version, sizeof(version), 1, file);
    fread(&timestamp, sizeof(timestamp), 1, file);
    
    if (strcmp(magic, MAGIC_NUMBER) != 0 || version != SAVE_VERSION) {
        printf("Invalid or incompatible save file\n");
        fclose(file);
        return false;
    }

    // 2. Load player state
    int32_t playerGridX, playerGridY;
    float playerPosX, playerPosY;
    
    fread(&playerGridX, sizeof(playerGridX), 1, file);
    fread(&playerGridY, sizeof(playerGridY), 1, file);
    fread(&playerPosX, sizeof(playerPosX), 1, file);
    fread(&playerPosY, sizeof(playerPosY), 1, file);

    // Load player skills
    float constructionExp;
    fread(&constructionExp, sizeof(constructionExp), 1, file);
    player.skills.constructionExp = constructionExp;

    // 3. Load all structures
    uint32_t structureCount;
    fread(&structureCount, sizeof(structureCount), 1, file);
    
    for (uint32_t i = 0; i < structureCount; i++) {
        uint16_t structX, structY;
        uint8_t flags;
        float texX, texY;

        fread(&structX, sizeof(structX), 1, file);
        fread(&structY, sizeof(structY), 1, file);
        fread(&flags, sizeof(flags), 1, file);
        fread(&texX, sizeof(texX), 1, file);
        fread(&texY, sizeof(texY), 1, file);

    if (structX < GRID_SIZE && structY < GRID_SIZE) {
        grid[structY][structX].hasWall = true;
        bool isDoorOpen = (flags & 2) != 0;
        grid[structY][structX].isDoorOpen = isDoorOpen;
        grid[structY][structX].isWalkable = false;  // TODO change attrib name to differ from getter function
        
        if (isDoorOpen && (grid[structY][structX].wallTexY == 1.0f/6.0f)) {  // TODO: use the constant def in structures 
            grid[structY][structX].isWalkable = true;  // Only then make it walkable
        }
        
        grid[structY][structX].wallTexX = texX;
        grid[structY][structX].wallTexY = texY;
        
        printf("Loaded structure at (%d,%d): isDoor=%d, isWalkable=%d texY=%f\n", 
            structX, structY, isDoorOpen, grid[structY][structX].isWalkable, texY);
    }
    }

    // 4. Load enclosures
    uint32_t enclosureCount;
    fread(&enclosureCount, sizeof(enclosureCount), 1, file);
    printf("Loading %d enclosures\n", enclosureCount);
    
    for (uint32_t i = 0; i < enclosureCount; i++) {
        EnclosureData enclosure = {0};

        fread(&enclosure.hash, sizeof(enclosure.hash), 1, file);
        fread(&enclosure.boundaryCount, sizeof(enclosure.boundaryCount), 1, file);
        fread(&enclosure.interiorCount, sizeof(enclosure.interiorCount), 1, file);
        fread(&enclosure.totalArea, sizeof(enclosure.totalArea), 1, file);
        fread(&enclosure.centerPoint, sizeof(enclosure.centerPoint), 1, file);
        fread(&enclosure.doorCount, sizeof(enclosure.doorCount), 1, file);
        fread(&enclosure.wallCount, sizeof(enclosure.wallCount), 1, file);
        fread(&enclosure.isValid, sizeof(enclosure.isValid), 1, file);

        enclosure.boundaryTiles = malloc(enclosure.boundaryCount * sizeof(Point));
        enclosure.interiorTiles = malloc(enclosure.interiorCount * sizeof(Point));

        if (!enclosure.boundaryTiles || !enclosure.interiorTiles) {
            printf("Failed to allocate enclosure memory\n");
            free(enclosure.boundaryTiles);
            free(enclosure.interiorTiles);
            continue;
        }

        fread(enclosure.boundaryTiles, sizeof(Point), enclosure.boundaryCount, file);
        fread(enclosure.interiorTiles, sizeof(Point), enclosure.interiorCount, file);

        addEnclosure(&globalEnclosureManager, &enclosure);
        printf("Loaded enclosure: hash=%lu, boundaryCount=%d, isValid=%d\n",
               enclosure.hash, enclosure.boundaryCount, enclosure.isValid);
        
        free(enclosure.boundaryTiles);
        free(enclosure.interiorTiles);
    }

    // 5. Apply player state
    atomic_store(&player.entity.gridX, playerGridX);
    atomic_store(&player.entity.gridY, playerGridY);
    atomic_store(&player.entity.posX, playerPosX);
    atomic_store(&player.entity.posY, playerPosY);

    atomic_store(&player.cameraTargetX, playerPosX);
    atomic_store(&player.cameraTargetY, playerPosY);
    atomic_store(&player.cameraCurrentX, playerPosX);
    atomic_store(&player.cameraCurrentY, playerPosY);

    atomic_store(&player.entity.targetGridX, playerGridX);
    atomic_store(&player.entity.targetGridY, playerGridY);
    atomic_store(&player.entity.finalGoalX, playerGridX);
    atomic_store(&player.entity.finalGoalY, playerGridY);
    atomic_store(&player.entity.needsPathfinding, false);

    player.hasBuildTarget = false;
    player.targetBuildX = 0;
    player.targetBuildY = 0;

    fclose(file);
    return true;
}
void CleanupBeforeLoad(void) {
    // Cleanup existing state before loading
    CleanupEntities();
    cleanupEnclosureManager(&globalEnclosureManager);
    if (globalChunkManager) {
        cleanupChunkManager(globalChunkManager);
        free(globalChunkManager);
        globalChunkManager = NULL;
    }
}

bool InitializeFromSave(const char* filename) {
    CleanupBeforeLoad();
    InitializeGameState(false);  // false = loading save
    return loadGameState(filename);
}