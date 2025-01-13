#include "inventory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * CreateInventory
 * 
 * Allocates and initializes a new inventory.
 * O(1) operation.
 *
 * @return Pointer to new Inventory or NULL if allocation failed
 */
Inventory* CreateInventory(void) {
    Inventory* inv = (Inventory*)malloc(sizeof(Inventory));
    if (!inv) {
        fprintf(stderr, "Failed to allocate inventory\n");
        return NULL;
    }

    memset(inv->slots, 0, sizeof(inv->slots));
    inv->selectedSlot = 0;
    inv->slotCount = 0;
    return inv;
}

/*
 * DestroyInventory
 * 
 * Frees inventory and all contained items.
 * O(n) where n is inventory size.
 *
 * @param[in] inv Inventory to destroy
 */
void DestroyInventory(Inventory* inv) {
    if (!inv) return;
    
    // Clean up all items before freeing the inventory
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (inv->slots[i]) {
            DestroyItem(inv->slots[i]);
            inv->slots[i] = NULL;
        }
    }

    free(inv);
}

/*
 * FindEmptySlot
 * 
 * Finds first empty slot or stackable slot for item.
 * O(n) worst case, but optimized for common cases.
 *
 * @param[in] inv Inventory to search
 * @param[in] item Item to find slot for
 * @return Slot index or -1 if no valid slot found
 */
static int FindEmptySlot(const Inventory* inv, const Item* item) {
    if (!inv || !item) return -1;

    // First try to stack if item is stackable
    if (IsStackable(item)) {
        for (int i = 0; i < INVENTORY_SIZE; i++) {
            Item* slot = inv->slots[i];
            if (slot && slot->type == item->type && 
                slot->count < GetMaxStack(slot->type)) {
                return i;
            }
        }
    }

    // Then look for empty slot
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (!inv->slots[i]) {
            return i;
        }
    }

    return -1;
}

/*
 * AddItem
 * 
 * Adds item to inventory, handling stacking.
 * O(n) worst case for finding slot.
 *
 * @param[in,out] inv Target inventory
 * @param[in] item Item to add
 * @return true if added successfully
 */
bool AddItem(Inventory* inv, Item* item) {
    if (!inv || !item) {
        printf("AddItem failed: NULL inventory or item\n");
        return false;
    }

    int slot = FindEmptySlot(inv, item);
    if (slot == -1) {
        printf("AddItem failed: No empty slot found (total slots: %d)\n", inv->slotCount);
        return false;
    }

    printf("Found slot %d for item type %d\n", slot, item->type);

    if (inv->slots[slot]) {
        // Stack with existing item
        uint16_t maxStack = GetMaxStack(item->type);
        uint16_t canAdd = maxStack - inv->slots[slot]->count;
        uint16_t toAdd = (item->count <= canAdd) ? item->count : canAdd;
        
        inv->slots[slot]->count += toAdd;
        item->count -= toAdd;
        
        if (item->count == 0) {
            DestroyItem(item);
            return true;
        }
        return false; // Still has remaining items
    } else {
        // New slot
        inv->slots[slot] = item;
        if (inv->slotCount < INVENTORY_SIZE) {
            inv->slotCount++;
        }
        return true;
    }
}

/*
 * RemoveItem
 * 
 * Removes and returns item from slot.
 * O(1) operation.
 *
 * @param[in,out] inv Source inventory
 * @param[in] slot Slot to remove from
 * @return Removed item or NULL
 */
Item* RemoveItem(Inventory* inv, uint8_t slot) {
    if (!inv || slot >= INVENTORY_SIZE) return NULL;
    
    Item* item = inv->slots[slot];
    inv->slots[slot] = NULL;
    
    if (item && inv->slotCount > 0) {
        inv->slotCount--;
    }
    
    return item;
}

/*
 * SwapSlots
 * 
 * Swaps items between two slots.
 * O(1) operation.
 *
 * @param[in,out] inv Inventory to modify
 * @param[in] slot1 First slot
 * @param[in] slot2 Second slot
 * @return true if swap succeeded
 */
bool SwapSlots(Inventory* inv, uint8_t slot1, uint8_t slot2) {
    if (!inv || slot1 >= INVENTORY_SIZE || slot2 >= INVENTORY_SIZE) {
        return false;
    }

    Item* temp = inv->slots[slot1];
    inv->slots[slot1] = inv->slots[slot2];
    inv->slots[slot2] = temp;
    return true;
}

/*
 * SelectSlot
 * 
 * Changes selected hotbar slot.
 * O(1) operation.
 *
 * @param[in,out] inv Inventory to modify
 * @param[in] slot Slot to select
 * @return true if selection changed
 */
bool SelectSlot(Inventory* inv, uint8_t slot) {
    if (!inv || slot >= INVENTORY_HOTBAR_SIZE) return false;
    inv->selectedSlot = slot;
    return true;
}

/*
 * GetSelectedItem
 * 
 * Gets item from currently selected slot.
 * O(1) operation.
 *
 * @param[in] inv Source inventory
 * @return Selected item or NULL
 */
Item* GetSelectedItem(const Inventory* inv) {
    if (!inv) return NULL;
    return inv->slots[inv->selectedSlot];
}

/*
 * CanAddItem 
 *
 * Checks if item can be added to inventory.
 * O(n) operation but no modifications.
 *
 * @param[in] inv Target inventory
 * @param[in] item Item to check
 * @return true if item can be added
 */
bool CanAddItem(const Inventory* inv, const Item* item) {
    if (!inv || !item) return false;
    return FindEmptySlot(inv, item) != -1;
}