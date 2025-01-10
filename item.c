// item.c
#include "item.h"
#include <stdlib.h>
#include <stdio.h>

/*
 * CreateItem
 * 
 * Creates a new item of specified type.
 *
 * @param[in] type The type of item to create
 * @return Pointer to new Item or NULL if allocation failed
 */
Item* CreateItem(ItemType type) {
    Item* item = (Item*)malloc(sizeof(Item));
    if (!item) {
        fprintf(stderr, "Failed to allocate item\n");
        return NULL;
    }

    item->type = type;
    item->count = 1;
    item->durability = 100;
    item->flags = 0;

    // Set default flags based on type
    switch(type) {
        case ITEM_WOOD:
        case ITEM_STONE:
            item->flags |= ITEM_FLAG_STACKABLE;
            break;
        case ITEM_TOOL:
            item->flags |= ITEM_FLAG_EQUIPABLE;
            break;
        default:
            break;
    }

    return item;
}

/*
 * DestroyItem
 * 
 * Frees an item.
 *
 * @param[in] item Item to destroy
 */
void DestroyItem(Item* item) {
    free(item);
}

/*
 * IsStackable
 * 
 * Checks if an item can be stacked.
 *
 * @param[in] item Item to check
 * @return true if item can be stacked
 */
bool IsStackable(const Item* item) {
    if (!item) return false;
    return (item->flags & ITEM_FLAG_STACKABLE) != 0;
}

/*
 * GetMaxStack
 * 
 * Gets maximum stack size for an item type.
 *
 * @param[in] type ItemType to check
 * @return Maximum stack size
 */
uint16_t GetMaxStack(ItemType type) {
    switch(type) {
        case ITEM_WOOD:
        case ITEM_STONE:
            return 64;
        case ITEM_TOOL:
            return 1;
        default:
            return 1;
    }
}