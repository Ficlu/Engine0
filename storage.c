// storage.c
#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include "structures.h"
#include "grid.h"
#include "player.h"
#include "inventory.h"
// Global storage manager (similar to enclosure manager pattern)
StorageManager globalStorageManager;

void initStorageManager(StorageManager* manager) {
    printf("Initializing storage manager...\n");
    
    manager->capacity = 16;  // Initial capacity
    manager->count = 0;
    manager->crates = malloc(manager->capacity * sizeof(CrateInventory));
    
    if (!manager->crates) {
        fprintf(stderr, "Failed to allocate storage manager crates array\n");
        exit(1);
    }
    
    // Initialize the crates array
    memset(manager->crates, 0, manager->capacity * sizeof(CrateInventory));
    
    printf("Storage manager initialized with capacity %zu\n", manager->capacity);
}

bool isPlantMaterial(MaterialType type) {
    return (type == MATERIAL_FERN || type == MATERIAL_TREE);
}

bool canAddToCrate(CrateInventory* crate, MaterialType type, uint16_t amount) {
    if (!crate || !isPlantMaterial(type)) {
        return false;
    }
    
    if (crate->totalItems + amount > crate->maxCapacity) {
        return false;
    }
    
    StoredMaterial* stack = &crate->items[type];
    return stack->count + amount <= stack->maxStack;
}

bool addToCrate(CrateInventory* crate, MaterialType type, uint16_t amount) {
    if (!canAddToCrate(crate, type, amount)) {
        printf("Cannot add %d items of type %d to crate\n", amount, type);
        return false;
    }
    
    StoredMaterial* stack = &crate->items[type];
    stack->count += amount;
    crate->totalItems += amount;
    printf("Added %d items of type %d to crate. New total: %d\n", 
           amount, type, crate->totalItems);
    return true;
}

uint16_t removeFromCrate(CrateInventory* crate, MaterialType type, uint16_t amount) {
    if (!crate || !isPlantMaterial(type)) {
        return 0;
    }

    StoredMaterial* stack = &crate->items[type];
    uint16_t removed = (amount > stack->count) ? stack->count : amount;
    
    stack->count -= removed;
    crate->totalItems -= removed;
    printf("Removed %d items of type %d from crate. Remaining: %d\n",
           removed, type, stack->count);
    return removed;
}

CrateInventory* createCrate(int gridX, int gridY) {
    if (globalStorageManager.count >= globalStorageManager.capacity) {
        size_t newCapacity = globalStorageManager.capacity * 2;
        CrateInventory* newArray = realloc(globalStorageManager.crates, 
                                         newCapacity * sizeof(CrateInventory));
        if (!newArray) {
            fprintf(stderr, "Failed to resize storage manager\n");
            return NULL;
        }
        globalStorageManager.crates = newArray;
        globalStorageManager.capacity = newCapacity;
    }

    CrateInventory* crate = &globalStorageManager.crates[globalStorageManager.count++];
    
    // Initialize crate
    memset(crate, 0, sizeof(CrateInventory));
    crate->maxCapacity = 100;  // We can tune this value
    crate->crateId = gridY * GRID_SIZE + gridX;  // Unique ID from position
    crate->isOpen = false;

    // Initialize material stacks
    for (int i = 0; i < MATERIAL_COUNT; i++) {
        crate->items[i].type = i;
        crate->items[i].maxStack = 50;  // We can tune this value
        crate->items[i].count = 0;
    }

    printf("Created new crate at (%d,%d) with ID: %u\n", 
           gridX, gridY, crate->crateId);
    return crate;
}

void cleanupCrateContents(CrateInventory* crate) {
    if (!crate) return;
    
    // Clean up stored materials
    for (int j = 0; j < MATERIAL_COUNT; j++) {
        StoredMaterial* material = &crate->items[j];
        // If we have any dynamic allocations per material, clean them here
        material->count = 0;
        material->maxStack = 0;
    }
    
    // Reset crate state
    crate->totalItems = 0;
    crate->maxCapacity = 0;
    crate->isOpen = false;
}

void removeCrateFromGrid(uint32_t crateId) {
    int gridX = crateId % GRID_SIZE;
    int gridY = crateId / GRID_SIZE;
    
    if (gridX >= 0 && gridX < GRID_SIZE && 
        gridY >= 0 && gridY < GRID_SIZE) {
        grid[gridY][gridX].structureType = STRUCTURE_NONE;
        grid[gridY][gridX].materialType = MATERIAL_NONE;
        GRIDCELL_SET_WALKABLE(grid[gridY][gridX], true);
    }
}

// Main cleanup function
void cleanupStorageManager(StorageManager* manager) {
    if (!manager) return;
    
    printf("Starting storage manager cleanup...\n");
    
    // Clean up each crate's contents
    for (size_t i = 0; i < manager->count; i++) {
        CrateInventory* crate = &manager->crates[i];
        
        // Log cleanup for debugging
        printf("Cleaning up crate %zu at position (%d, %d)\n", 
               i, crate->crateId % GRID_SIZE, crate->crateId / GRID_SIZE);
        
        // Clean up crate contents
        cleanupCrateContents(crate);
        
        // Remove grid references
        removeCrateFromGrid(crate->crateId);
        
        // Close any open UI elements for this crate
        if (crate->isOpen) {
            // Clean up any UI resources
            crate->isOpen = false;
        }
    }
    
    // Free the main crates array
    printf("Freeing crates array with %zu crates\n", manager->count);
    free(manager->crates);
    manager->crates = NULL;
    manager->count = 0;
    manager->capacity = 0;
    
    printf("Storage manager cleanup complete\n");
}
CrateInventory* findCrate(uint32_t crateId) {
    for (size_t i = 0; i < globalStorageManager.count; i++) {
        if (globalStorageManager.crates[i].crateId == crateId) {
            return &globalStorageManager.crates[i];
        }
    }
    return NULL;
}

MaterialType itemTypeToMaterialType(ItemType type) {
    switch(type) {
        case ITEM_FERN:
            return MATERIAL_FERN;
        // Add other cases as needed
        default:
            return MATERIAL_NONE;
    }
}

bool removeFromCrateToInventory(CrateInventory* crate, MaterialType type, Inventory* targetInventory) {
    if (!crate || !targetInventory || type >= MATERIAL_COUNT) {
        return false;
    }

    // Check if we have any of this material
    if (crate->items[type].count == 0) {
        return false;
    }

    // Create corresponding item
    Item* item = NULL;
    switch(type) {
        case MATERIAL_FERN:
            item = CreateItem(ITEM_FERN);
            break;
        case MATERIAL_TREE:
            // Add other cases as needed
            break;
        default:
            return false;
    }

    if (!item) {
        return false;
    }

    // Try to add to inventory
    if (AddItem(targetInventory, item)) {
        // Successfully added to inventory, remove from crate
        crate->items[type].count--;
        return true;
    } else {
        // Failed to add to inventory, cleanup item
        DestroyItem(item);
        return false;
    }
}

void destroyCrate(uint32_t crateId) {
    CrateInventory* crate = findCrate(crateId);
    if (!crate) return;
    
    printf("Destroying crate at ID %u\n", crateId);
    
    // Clean up contents
    cleanupCrateContents(crate);
    
    // Remove from grid
    removeCrateFromGrid(crateId);
    
    // Remove from storage manager
    for (size_t i = 0; i < globalStorageManager.count; i++) {
        if (globalStorageManager.crates[i].crateId == crateId) {
            // Move last crate to this position if not last
            if (i < globalStorageManager.count - 1) {
                globalStorageManager.crates[i] = 
                    globalStorageManager.crates[globalStorageManager.count - 1];
            }
            globalStorageManager.count--;
            break;
        }
    }
}