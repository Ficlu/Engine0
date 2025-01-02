// player.c

#include "player.h"
#include "gameloop.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

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

    // Initialize new build-related fields
    player->targetBuildX = 0;
    player->targetBuildY = 0;
    player->hasBuildTarget = false;

    atomic_store(&player->cameraTargetX, atomic_load(&player->entity.posX));
    atomic_store(&player->cameraTargetY, atomic_load(&player->entity.posY));
    atomic_store(&player->cameraCurrentX, atomic_load(&player->entity.posX));
    atomic_store(&player->cameraCurrentY, atomic_load(&player->entity.posY));
    player->cameraSpeed = 0.1f;

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

    printf("Player initialized at (%d, %d)\n", atomic_load(&player->entity.gridX), atomic_load(&player->entity.gridY));
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
            // Place the wall
            grid[player->targetBuildY][player->targetBuildX].hasWall = true;
            grid[player->targetBuildY][player->targetBuildX].isWalkable = false;
            printf("Reached build location - placed wall at: %d, %d\n", 
                   player->targetBuildX, player->targetBuildY);
            
            // Clear the build target
            player->hasBuildTarget = false;
        }
    }

    // Camera follow logic
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
}
