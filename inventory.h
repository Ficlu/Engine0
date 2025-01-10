#ifndef INVENTORY_H
#define INVENTORY_H

#include "item.h"
#include <stdbool.h>

#define INVENTORY_HOTBAR_SIZE 8
#define INVENTORY_SIZE 32

typedef struct {
    Item* slots[INVENTORY_SIZE];
    uint8_t selectedSlot;  // Currently selected hotbar slot
    uint8_t slotCount;     // Number of slots actually used
} Inventory;

// Core inventory operations - all should be O(1) where possible
Inventory* CreateInventory(void);
void DestroyInventory(Inventory* inv);
bool AddItem(Inventory* inv, Item* item);
Item* RemoveItem(Inventory* inv, uint8_t slot);
bool CanAddItem(const Inventory* inv, const Item* item);
bool SwapSlots(Inventory* inv, uint8_t slot1, uint8_t slot2);

// Quick slot operations
bool SelectSlot(Inventory* inv, uint8_t slot);
Item* GetSelectedItem(const Inventory* inv);

#endif