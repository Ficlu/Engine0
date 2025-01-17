#ifndef TEXTURE_COORDS_H
#define TEXTURE_COORDS_H

#include <stdbool.h>

// Texture atlas dimensions
#define ATLAS_COLS 3
#define ATLAS_ROWS 6

// Basic UV coordinate structure
typedef struct {
    float u1, v1;  // Top-left UV coordinates
    float u2, v2;  // Bottom-right UV coordinates
} TextureCoords;

// Core texture info structure
typedef struct {
    const char* id;           
    TextureCoords coords;     
    int atlasX, atlasY;      // Position in texture atlas grid
} TextureInfo;

// Hash table entry for O(1) lookups
typedef struct TextureHashEntry {
    const char* key;
    TextureCoords value;
    struct TextureHashEntry* next;
} TextureHashEntry;

// Main texture manager
typedef struct {
    TextureHashEntry** table;
    int size;           // Size of hash table
    float loadFactor;   // For automatic resizing
} TextureManager;

// Global texture manager instance
extern TextureManager* gTextureManager;

// Core functions
void initTextureManager(int initialSize);
void cleanupTextureManager(void);
bool registerTexture(const char* id, int atlasX, int atlasY);
TextureCoords* getTextureCoords(const char* id);

// Utility functions
void dumpTextureRegistry(void);  // For debugging
bool validateTextureId(const char* id);
void resizeTextureManager(int newSize);

// Error handling
typedef enum {
    TEXTURE_SUCCESS,
    TEXTURE_NOT_FOUND,
    TEXTURE_INVALID_COORDS,
    TEXTURE_DUPLICATE_ID,
    TEXTURE_MEMORY_ERROR
} TextureError;

extern TextureError lastTextureError;
const char* getTextureErrorString(TextureError error);
void initializeDefaultTextures(void);
#endif // TEXTURE_COORDS_H