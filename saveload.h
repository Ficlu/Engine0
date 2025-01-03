// saveload.h
#ifndef SAVELOAD_H
#define SAVELOAD_H

#include <stdbool.h>
#include <stdint.h>

// Version 1 of save format
#define SAVE_VERSION 1
#define MAGIC_NUMBER "SAV1"

bool saveGameState(const char* filename);
bool loadGameState(const char* filename);

#endif