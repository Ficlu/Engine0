// player.h
#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"
#include "structure_types.h"
#include "structures.h"
#include "inventory.h"

// Define available skills
typedef enum {
    SKILL_CONSTRUCTION = 0,
    SKILL_FORAGING = 1,
    SKILL_COUNT
} SkillType;

// Skill data structure
typedef struct {
    uint32_t levels[SKILL_COUNT];     // Whole number levels for each skill
    float experience[SKILL_COUNT];     // Fractional experience for smooth bar movement
    SkillType lastUpdatedSkill;       // Tracks which skill most recently gained exp
} Skills;

typedef struct Player {
    Entity entity;
    float cameraTargetX;
    float cameraTargetY;
    float cameraCurrentX;
    float cameraCurrentY;
    float cameraSpeed;
    float lookAheadX;
    float lookAheadY;
    float zoomFactor;
    int targetBuildX;
    int targetBuildY;
    bool hasBuildTarget;
    
    StructureType pendingBuildType;
    Skills skills;
    Inventory* inventory;
} Player;

// Function declarations
void InitPlayerInventory(Player* player);
void CleanupPlayerInventory(Player* player);
void InitPlayer(Player* player, int startGridX, int startGridY, float speed);
void UpdatePlayer(Player* player, Entity** allEntities, int entityCount);
void CleanupPlayer(Player* player);
void awardForagingExp(Player* player, const Item* item);
// Skill-related function declarations
float getSkillExp(const Player* player, SkillType skill);
uint32_t getSkillLevel(const Player* player, SkillType skill);
void awardSkillExp(Player* player, SkillType skill, float amount);
const char* getSkillName(SkillType skill);
void awardConstructionExp(Player* player, const EnclosureData* enclosure);

#endif // PLAYER_H