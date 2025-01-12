#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"
#include "structure_types.h"

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
} Skills;


#include "inventory.h"  // Add this

typedef struct {
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
    Inventory* inventory;  // Add this
} Player;

// Add these new function declarations
void InitPlayerInventory(Player* player);
void CleanupPlayerInventory(Player* player);

// Existing declarations
void InitPlayer(Player* player, int startGridX, int startGridY, float speed);
void UpdatePlayer(Player* player, Entity** allEntities, int entityCount);
void CleanupPlayer(Player* player);


// New skill-related function declarations
float getSkillExp(const Player* player, SkillType skill);
uint32_t getSkillLevel(const Player* player, SkillType skill);
void awardSkillExp(Player* player, SkillType skill, float amount);
const char* getSkillName(SkillType skill);

#endif // PLAYER_H