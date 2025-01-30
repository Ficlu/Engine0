#ifndef STORAGE_H
#define STORAGE_H
#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdbool.h>
#include "structure_types.h"
#include "item.h"
#include "inventory.h"

typedef struct {
    MaterialType type;
    uint16_t count;
    uint16_t maxStack;
} StoredMaterial;

typedef struct {
    StoredMaterial items[MATERIAL_COUNT];
    uint32_t totalItems;
    uint32_t maxCapacity;
    bool isOpen;
    uint32_t crateId;
} CrateInventory;

typedef struct {
    CrateInventory* crates;
    size_t count;
    size_t capacity;
} StorageManager;

extern StorageManager globalStorageManager;

// Core storage functions
void initStorageManager(StorageManager* manager);
void cleanupStorageManager(StorageManager* manager);
bool isPlantMaterial(MaterialType type);

// Crate operations
CrateInventory* createCrate(int gridX, int gridY);
CrateInventory* findCrate(uint32_t crateId);
bool canAddToCrate(CrateInventory* crate, MaterialType type, uint16_t amount);
bool addToCrate(CrateInventory* crate, MaterialType type, uint16_t amount);
uint16_t removeFromCrate(CrateInventory* crate, MaterialType type, uint16_t amount);
MaterialType itemTypeToMaterialType(ItemType type);
bool removeFromCrateToInventory(CrateInventory* crate, MaterialType type, Inventory* targetInventory);

#endif // STORAGE_H