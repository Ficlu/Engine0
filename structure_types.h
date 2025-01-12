#ifndef STRUCTURE_TYPES_H
#define STRUCTURE_TYPES_H

typedef enum {
    STRUCTURE_NONE = 0,
    STRUCTURE_WALL = 1,
    STRUCTURE_DOOR = 2,
    STRUCTURE_PLANT = 3,
    STRUCTURE_COUNT
} StructureType;

typedef enum {
    MATERIAL_NONE = 0,
    // Building materials
    MATERIAL_WOOD = 1,
    RES2 = 2,
    RES1 = 3,
    RES3 = 4,
    RES4 = 5,
    RES5 = 6,
    RES6 = 7,
    RES7 = 8,
    RES8 = 9,
    RES9 = 10,
    RES10 = 11, 
    RES11 = 12,
    // Plant /types
    MATERIAL_FERN = 13,
    MATERIAL_COUNT
} MaterialType;

// Helper function declarations
const char* getStructureName(StructureType type);
const char* getMaterialName(MaterialType type);

#endif // STRUCTURE_TYPES_H