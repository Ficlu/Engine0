// enclosure_types.h
#ifndef ENCLOSURE_TYPES_H
#define ENCLOSURE_TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int x, y;
} Point;

typedef struct {
    Point* boundaryTiles;
    int boundaryCount;
    Point* interiorTiles;
    int interiorCount;
    Point centerPoint;
    uint64_t hash;
    int totalArea;
    int wallCount;
    int doorCount;
    bool isValid;
} EnclosureData;

#endif // ENCLOSURE_TYPES_H