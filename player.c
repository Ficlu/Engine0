// player.c

#include "player.h"
#include "gameloop.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "structures.h"
/*
 * InitPlayer
 *
 * Initializes a player entity with given starting position and speed.
 *
 * @param[out] player Pointer to the Player structure to initialize
 * @param[in] startGridX Starting X position on the grid
 * @param[in] startGridY Starting Y position on the grid
 * @param[in] speed Movement speed of the player
 *
 * @pre player is a valid pointer to a Player structure
 * @pre startGridX and startGridY are within valid grid bounds
 * @pre speed is a positive float value
 */
void InitPlayer(Player* player, int startGridX, int startGridY, float speed) {
    atomic_store(&player->entity.gridX, startGridX);
    atomic_store(&player->entity.gridY, startGridY);
    player->entity.speed = speed;
    
    float tempPosX, tempPosY;
    WorldToScreenCoords(atomic_load(&player->entity.gridX), atomic_load(&player->entity.gridY), 0, 0, 1, &tempPosX, &tempPosY);
    atomic_store(&player->entity.posX, tempPosX);
    atomic_store(&player->entity.posY, tempPosY);

    for (int i = 0; i < SKILL_COUNT; i++) {
        player->skills.levels[i] = 0;
        player->skills.experience[i] = 0.0f;
    }
    
    player->skills.lastUpdatedSkill = SKILL_CONSTRUCTION; 
    atomic_store(&player->entity.targetGridX, startGridX);
    atomic_store(&player->entity.targetGridY, startGridY);
    atomic_store(&player->entity.finalGoalX, startGridX);
    atomic_store(&player->entity.finalGoalY, startGridY);
    atomic_store(&player->entity.needsPathfinding, false);
    player->entity.cachedPath = NULL;
    player->entity.cachedPathLength = 0;
    player->entity.currentPathIndex = 0;
    player->zoomFactor = 3.0f;
    player->entity.isPlayer = true;

    // Initialize build-related fields
    player->targetBuildX = 0;
    player->targetBuildY = 0;
    player->hasBuildTarget = false;

    // Initialize harvest-related fields
    player->targetHarvestX = 0;
    player->targetHarvestY = 0;
    player->hasHarvestTarget = false;
    player->pendingHarvestType = 0;

    atomic_store(&player->cameraTargetX, atomic_load(&player->entity.posX));
    atomic_store(&player->cameraTargetY, atomic_load(&player->entity.posY));
    atomic_store(&player->cameraCurrentX, atomic_load(&player->entity.posX));
    atomic_store(&player->cameraCurrentY, atomic_load(&player->entity.posY));
    player->cameraSpeed = 0.1f;

    // Initialize inventory
    player->inventory = CreateInventory();
    if (!player->inventory) {
        fprintf(stderr, "Failed to create player inventory\n");
        exit(1);
    }

    int tempNearestX, tempNearestY;
    findNearestWalkableTile(atomic_load(&player->entity.posX), atomic_load(&player->entity.posY), &tempNearestX, &tempNearestY);
    atomic_store(&player->entity.gridX, tempNearestX);
    atomic_store(&player->entity.gridY, tempNearestY);

    WorldToScreenCoords(atomic_load(&player->entity.gridX), atomic_load(&player->entity.gridY), 0, 0, 1, &tempPosX, &tempPosY);
    atomic_store(&player->entity.posX, tempPosX);
    atomic_store(&player->entity.posY, tempPosY);

    atomic_store(&player->cameraTargetX, atomic_load(&player->entity.posX));
    atomic_store(&player->cameraTargetY, atomic_load(&player->entity.posY));
    atomic_store(&player->cameraCurrentX, atomic_load(&player->entity.posX));
    atomic_store(&player->cameraCurrentY, atomic_load(&player->entity.posY));

    printf("Player initialized at (%d, %d) with inventory\n", 
           atomic_load(&player->entity.gridX), 
           atomic_load(&player->entity.gridY));
}
/*
 * UpdatePlayer
 *
 * Updates the player's state, including position and camera.
 *
 * @param[in,out] player Pointer to the Player structure to update
 * @param[in] allEntities Array of pointers to all entities in the game
 * @param[in] entityCount Number of entities in the allEntities array
 *
 * @pre player is a valid pointer to a Player structure
 * @pre allEntities is a valid array of Entity pointers
 * @pre entityCount is a positive integer
 */
void UpdatePlayer(Player* player, Entity** allEntities, int entityCount) {
    UpdateEntity(&player->entity, allEntities, entityCount);

    // Check if we have a build target and have reached it
    if (player->hasBuildTarget) {
        int currentX = atomic_load(&player->entity.gridX);
        int currentY = atomic_load(&player->entity.gridY);
        
        bool isNearTarget = (
            abs(currentX - player->targetBuildX) <= 1 && 
            abs(currentY - player->targetBuildY) <= 1
        );

        if (isNearTarget) {
            bool placed = placeStructure(player->pendingBuildType, 
                                       player->targetBuildX, 
                                       player->targetBuildY);
            printf("Reached build location - placement %s at: %d, %d\n",
                   placed ? "succeeded" : "failed",
                   player->targetBuildX, player->targetBuildY);
            
            player->hasBuildTarget = false;
        }
    }

    // NEW: Check if we have a harvest target and have reached it
    if (player->hasHarvestTarget) {
        int currentX = atomic_load(&player->entity.gridX);
        int currentY = atomic_load(&player->entity.gridY);
        
        bool isNearTarget = (
            abs(currentX - player->targetHarvestX) <= 1 && 
            abs(currentY - player->targetHarvestY) <= 1
        );

        if (isNearTarget) {
            // Create new item based on what we're harvesting
            Item* harvestedItem = NULL;
            switch(player->pendingHarvestType) {
                case MATERIAL_FERN:
                    harvestedItem = CreateItem(ITEM_FERN);
                    break;
                // Add other harvestable types here
            }

            if (harvestedItem) {
                bool added = AddItem(player->inventory, harvestedItem);
                if (added) {
                    // Award experience before clearing the cell
                    awardForagingExp(player, harvestedItem);
                    
                    // Clear the harvested object from the grid
                    grid[player->targetHarvestY][player->targetHarvestX].structureType = STRUCTURE_NONE;
                    grid[player->targetHarvestY][player->targetHarvestX].materialType = MATERIAL_NONE;
                    GRIDCELL_SET_WALKABLE(grid[player->targetHarvestY][player->targetHarvestX], true);
                    printf("Successfully harvested at: %d, %d\n", 
                           player->targetHarvestX, player->targetHarvestY);
                } else {
                    printf("Failed to add harvested item to inventory\n");
                    DestroyItem(harvestedItem);
                }
            }
            
            player->hasHarvestTarget = false;
            player->pendingHarvestType = 0;
        }
    }

    // Camera follow logic (unchanged)
    float playerPosX = atomic_load(&player->entity.posX);
    float playerPosY = atomic_load(&player->entity.posY);

    float dx = playerPosX - player->cameraCurrentX;
    float dy = playerPosY - player->cameraCurrentY;
    float lookAheadFactor = 1.0f;

    player->lookAheadX = dx * lookAheadFactor;
    player->lookAheadY = dy * lookAheadFactor;

    player->cameraTargetX = playerPosX + player->lookAheadX;
    player->cameraTargetY = playerPosY + player->lookAheadY;

    float cameraSmoothFactor = 0.05f;
    player->cameraCurrentX += (player->cameraTargetX - player->cameraCurrentX) * cameraSmoothFactor;
    player->cameraCurrentY += (player->cameraTargetY - player->cameraCurrentY) * cameraSmoothFactor;
}

/*
 * CleanupPlayer
 *
 * Frees any dynamically allocated memory associated with the player.
 *
 * @param[in,out] player Pointer to the Player structure to clean up
 *
 * @pre player is a valid pointer to a Player structure
 */
void CleanupPlayer(Player* player) {
    if (player == NULL) {
        fprintf(stderr, "Error: player pointer is NULL in CleanupPlayer\n");
        return;
    }

    if (player->entity.cachedPath) {
        free(player->entity.cachedPath);
        player->entity.cachedPath = NULL;
    }

    if (player->inventory) {
        DestroyInventory(player->inventory);
        player->inventory = NULL;
    }

    printf("Player cleanup completed\n");
}

void awardSkillExp(Player* player, SkillType skill, float amount) {
    if (!player || skill >= SKILL_COUNT) {
        fprintf(stderr, "Invalid skill award parameters\n");
        return;
    }

    // Store old values for progress tracking
    float oldExp = player->skills.experience[skill];
    int oldLevel = player->skills.levels[skill];
    
    // Add new exp
    player->skills.experience[skill] += amount;
    player->skills.lastUpdatedSkill = skill;  // Track this skill as latest updated
    
    // Calculate new level
    int newLevel = (int)(player->skills.experience[skill] / EXP_PER_LEVEL);
    player->skills.levels[skill] = newLevel;

    // Calculate progress percentages for logging
    float oldProgress = (oldExp - (oldLevel * EXP_PER_LEVEL)) / EXP_PER_LEVEL * 100.0f;
    float newProgress = (player->skills.experience[skill] - (newLevel * EXP_PER_LEVEL)) / EXP_PER_LEVEL * 100.0f;

    printf("\n=== %s Experience Award ===\n", getSkillName(skill));
    printf("Progress Update:\n");
    printf("- Total exp: %.1f -> %.1f\n", oldExp, player->skills.experience[skill]);
    printf("- Level: %d -> %d\n", oldLevel, newLevel);
    printf("- Progress to next level: %.1f%% -> %.1f%%\n", oldProgress, newProgress);
    
    if (newLevel > oldLevel) {
        printf("\n*** LEVEL UP! ***\n");
        printf("%s level increased from %d to %d!\n", getSkillName(skill), oldLevel, newLevel);
    }
    printf("==============================\n\n");
}

void awardConstructionExp(Player* player, const EnclosureData* enclosure) {
    // Base exp values
    const float BASE_WALL_EXP = 10.0f;
    const float BASE_DOOR_EXP = 25.0f;
    const float AREA_MULTIPLIER = 5.0f;

    // Calculate total exp
    float wallExp = BASE_WALL_EXP * enclosure->wallCount;
    float doorExp = BASE_DOOR_EXP * enclosure->doorCount;
    float areaExp = AREA_MULTIPLIER * enclosure->totalArea;
    float totalExp = wallExp + doorExp + areaExp;

    printf("\n=== Construction Experience Calculation ===\n");
    printf("Base Calculations:\n");
    printf("- Wall exp (%.0f per wall): %.0f (walls: %d)\n", BASE_WALL_EXP, wallExp, enclosure->wallCount);
    printf("- Door exp (%.0f per door): %.0f (doors: %d)\n", BASE_DOOR_EXP, doorExp, enclosure->doorCount);
    printf("- Area exp (%.0f per tile): %.0f (area: %d)\n", AREA_MULTIPLIER, areaExp, enclosure->totalArea);
    printf("Total exp to award: %.0f\n", totalExp);

    // Use the general skill exp system
    awardSkillExp(player, SKILL_CONSTRUCTION, totalExp);
}

void awardForagingExp(Player* player, const Item* item) {
    if (!player || !item) {
        printf("ERROR: Invalid player or item in awardForagingExp\n");
        return;
    }

    // Base exp values
    const float BASE_FERN_EXP = 25.0f;
    float totalExp = 0.0f;

    switch(item->type) {
        case ITEM_FERN:
            totalExp = BASE_FERN_EXP;
            break;
        default:
            printf("WARNING: Unhandled item type in forage exp calculation\n");
            return;
    }

    printf("\n=== Foraging Experience Calculation ===\n");
    printf("Item Type: %d\n", item->type);
    printf("Base Experience: %.0f\n", totalExp);

    // Use the general skill exp system
    awardSkillExp(player, SKILL_FORAGING, totalExp);
}
const char* getSkillName(SkillType skill) {
    switch (skill) {
        case SKILL_CONSTRUCTION:
            return "Construction";
        case SKILL_FORAGING:
            return "Foraging";
        default:
            return "Unknown Skill";
    }
}