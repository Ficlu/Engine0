// texture_coords.c
#include "texture_coords.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define ATLAS_COLS 3
#define ATLAS_ROWS 6
#define FNV_OFFSET 2166136261u
#define FNV_PRIME 16777619u

TextureManager* gTextureManager = NULL;
TextureError lastTextureError = TEXTURE_SUCCESS;

// FNV-1a hash function for strings
static unsigned int hashString(const char* str) {
    unsigned int hash = FNV_OFFSET;
    for (const char* c = str; *c; c++) {
        hash ^= (unsigned char)*c;
        hash *= FNV_PRIME;
    }
    return hash;
}

static TextureCoords calculateUVs(int gridX, int gridY) {
    float colWidth = 1.0f / ATLAS_COLS;
    float rowHeight = 1.0f / ATLAS_ROWS;
    
    return (TextureCoords){
        .u1 = gridX * colWidth,
        .v1 = gridY * rowHeight,
        .u2 = (gridX + 1) * colWidth,
        .v2 = (gridY + 1) * rowHeight
    };
}

void initTextureManager(int initialSize) {
    if (gTextureManager) {
        printf("Warning: TextureManager already initialized\n");
        return;
    }

    gTextureManager = malloc(sizeof(TextureManager));
    if (!gTextureManager) {
        lastTextureError = TEXTURE_MEMORY_ERROR;
        return;
    }

    gTextureManager->size = initialSize;
    gTextureManager->loadFactor = 0.0f;
    gTextureManager->table = calloc(initialSize, sizeof(TextureHashEntry*));
    
    if (!gTextureManager->table) {
        free(gTextureManager);
        gTextureManager = NULL;
        lastTextureError = TEXTURE_MEMORY_ERROR;
        return;
    }

    printf("TextureManager initialized with size %d\n", initialSize);
}

bool registerTexture(const char* id, int atlasX, int atlasY) {
    if (!gTextureManager || !id) return false;
    if (atlasX >= ATLAS_COLS || atlasY >= ATLAS_ROWS) {
        lastTextureError = TEXTURE_INVALID_COORDS;
        return false;
    }

    unsigned int hash = hashString(id) % gTextureManager->size;
    TextureHashEntry* entry = gTextureManager->table[hash];

    // Check for duplicates
    while (entry) {
        if (strcmp(entry->key, id) == 0) {
            lastTextureError = TEXTURE_DUPLICATE_ID;
            return false;
        }
        entry = entry->next;
    }

    // Create new entry
    TextureHashEntry* newEntry = malloc(sizeof(TextureHashEntry));
    if (!newEntry) {
        lastTextureError = TEXTURE_MEMORY_ERROR;
        return false;
    }

    newEntry->key = strdup(id);
    newEntry->value = calculateUVs(atlasX, atlasY);
    newEntry->next = gTextureManager->table[hash];
    gTextureManager->table[hash] = newEntry;

    // Update load factor and resize if needed
    gTextureManager->loadFactor = (float)++gTextureManager->loadFactor / gTextureManager->size;
    if (gTextureManager->loadFactor > 0.75f) {
        resizeTextureManager(gTextureManager->size * 2);
    }

    printf("Registered texture '%s' at atlas position (%d,%d)\n", id, atlasX, atlasY);
    return true;
}

TextureCoords* getTextureCoords(const char* id) {
    if (!gTextureManager || !id) return NULL;

    unsigned int hash = hashString(id) % gTextureManager->size;
    TextureHashEntry* entry = gTextureManager->table[hash];

    while (entry) {
        if (strcmp(entry->key, id) == 0) {
            return &entry->value;
        }
        entry = entry->next;
    }

    lastTextureError = TEXTURE_NOT_FOUND;
    return NULL;
}

void resizeTextureManager(int newSize) {
    TextureHashEntry** newTable = calloc(newSize, sizeof(TextureHashEntry*));
    if (!newTable) {
        lastTextureError = TEXTURE_MEMORY_ERROR;
        return;
    }

    // Rehash all entries
    for (int i = 0; i < gTextureManager->size; i++) {
        TextureHashEntry* entry = gTextureManager->table[i];
        while (entry) {
            TextureHashEntry* next = entry->next;
            unsigned int newHash = hashString(entry->key) % newSize;
            entry->next = newTable[newHash];
            newTable[newHash] = entry;
            entry = next;
        }
    }

    free(gTextureManager->table);
    gTextureManager->table = newTable;
    gTextureManager->size = newSize;
    printf("TextureManager resized to %d\n", newSize);
}

void cleanupTextureManager(void) {
    if (!gTextureManager) return;

    for (int i = 0; i < gTextureManager->size; i++) {
        TextureHashEntry* entry = gTextureManager->table[i];
        while (entry) {
            TextureHashEntry* next = entry->next;
            free((void*)entry->key);
            free(entry);
            entry = next;
        }
    }

    free(gTextureManager->table);
    free(gTextureManager);
    gTextureManager = NULL;
    printf("TextureManager cleaned up\n");
}

void dumpTextureRegistry(void) {
    if (!gTextureManager) return;

    printf("\n=== Texture Registry Dump ===\n");
    for (int i = 0; i < gTextureManager->size; i++) {
        TextureHashEntry* entry = gTextureManager->table[i];
        while (entry) {
            printf("ID: %s, UV: (%.3f,%.3f)-(%.3f,%.3f)\n",
                   entry->key,
                   entry->value.u1, entry->value.v1,
                   entry->value.u2, entry->value.v2);
            entry = entry->next;
        }
    }
    printf("===========================\n\n");
}

const char* getTextureErrorString(TextureError error) {
    switch (error) {
        case TEXTURE_SUCCESS: return "Success";
        case TEXTURE_NOT_FOUND: return "Texture not found";
        case TEXTURE_INVALID_COORDS: return "Invalid atlas coordinates";
        case TEXTURE_DUPLICATE_ID: return "Duplicate texture ID";
        case TEXTURE_MEMORY_ERROR: return "Memory allocation error";
        default: return "Unknown error";
    }
}

void initializeDefaultTextures(void) {
    initTextureManager(64);

    // Register all textures
    registerTexture("wall_front", 1, 3);
    registerTexture("wall_vertical", 0, 3);
    registerTexture("wall_top_left", 1, 2);
    registerTexture("wall_top_right", 2, 3);
    registerTexture("wall_bottom_left", 0, 2);
    registerTexture("wall_bottom_right", 2, 2);

    registerTexture("door_vertical", 2, 1);
    registerTexture("door_horizontal", 0, 1);
    registerTexture("door_vertical_open", 0, 0);
    registerTexture("door_horizontal_open", 1, 1);

    registerTexture("terrain_sand", 0, 4);
    registerTexture("terrain_water", 1, 4);
    registerTexture("terrain_grass", 2, 4);
    registerTexture("terrain_stone", 2, 5);

    registerTexture("player", 0, 5);
    registerTexture("enemy", 1, 5);
    registerTexture("item_fern", 1, 0);

    // Validate registration
    printf("\nValidating texture registration...\n");
    bool allValid = true;
    const char* testTextures[] = {
        "wall_front", "wall_vertical", "wall_top_left", "wall_top_right",
        "wall_bottom_left", "wall_bottom_right", "door_vertical", "door_horizontal",
        "door_vertical_open", "door_horizontal_open", "terrain_sand", "terrain_water",
        "terrain_grass", "terrain_stone", "player", "enemy", "item_fern"
    };

    for (size_t i = 0; i < sizeof(testTextures) / sizeof(testTextures[0]); i++) {
        TextureCoords* coords = getTextureCoords(testTextures[i]);
        if (!coords) {
            printf("ERROR: Failed to validate texture '%s'\n", testTextures[i]);
            allValid = false;
        }
    }

    if (allValid) {
        printf("All textures registered successfully!\n");
        dumpTextureRegistry();
    } else {
        printf("ERROR: Some textures failed to register properly!\n");
    }
}