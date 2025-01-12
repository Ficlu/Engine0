#ifndef ITEM_H
#define ITEM_H

#include <stdint.h>
#include <stdbool.h>
typedef enum {
    ITEM_NONE = 0,
    ITEM_WOOD,
    ITEM_STONE,
    ITEM_TOOL,
    ITEM_FERN,
    // Add more as needed
    ITEM_TYPE_COUNT
} ItemType;

typedef struct {
    ItemType type;
    uint16_t count;      // Stack size
    uint16_t durability; // For tools/weapons
    uint32_t flags;      // Bitfield for item properties
} Item;

#define ITEM_FLAG_STACKABLE    (1 << 0)
#define ITEM_FLAG_EQUIPABLE    (1 << 1)
#define ITEM_FLAG_CONSUMABLE   (1 << 2)

// Core item functions
Item* CreateItem(ItemType type);
void DestroyItem(Item* item);
bool IsStackable(const Item* item);
uint16_t GetMaxStack(ItemType type);

#endif