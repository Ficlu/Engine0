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
    player->entity.gridX = startGridX;
    player->entity.gridY = startGridY;
    player->entity.speed = speed;
    WorldToScreenCoords(startGridX, startGridY, 0, 0, 1, &player->entity.posX, &player->entity.posY);
    player->entity.targetGridX = startGridX;
    player->entity.targetGridY = startGridY;
    player->entity.finalGoalX = startGridX;
    player->entity.finalGoalY = startGridY;
    player->entity.needsPathfinding = false;
    player->entity.cachedPath = NULL;
    player->entity.cachedPathLength = 0;
    player->entity.currentPathIndex = 0;
    player->zoomFactor = 3.0f;
    player->entity.isPlayer = true;

    player->cameraTargetX = player->entity.posX;
    player->cameraTargetY = player->entity.posY;
    player->cameraCurrentX = player->entity.posX;
    player->cameraCurrentY = player->entity.posY;
    player->cameraSpeed = 0.1f;

    findNearestWalkableTile(player->entity.posX, player->entity.posY, &player->entity.gridX, &player->entity.gridY);
    WorldToScreenCoords(player->entity.gridX, player->entity.gridY, 0, 0, 1, &player->entity.posX, &player->entity.posY);
    player->cameraTargetX = player->entity.posX;
    player->cameraTargetY = player->entity.posY;
    player->cameraCurrentX = player->entity.posX;
    player->cameraCurrentY = player->entity.posY;

    printf("Player initialized at (%d, %d)\n", player->entity.gridX, player->entity.gridY);
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

    float dx = player->entity.posX - player->cameraCurrentX;
    float dy = player->entity.posY - player->cameraCurrentY;
    float lookAheadFactor = 0.5f;

    player->lookAheadX = dx * lookAheadFactor;
    player->lookAheadY = dy * lookAheadFactor;

    player->cameraTargetX = player->entity.posX + player->lookAheadX;
    player->cameraTargetY = player->entity.posY + player->lookAheadY;

    float smoothFactor = 0.05f;
    player->cameraCurrentX += (player->cameraTargetX - player->cameraCurrentX) * smoothFactor;
    player->cameraCurrentY += (player->cameraTargetY - player->cameraCurrentY) * smoothFactor;
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
    if (player->entity.cachedPath) {
        free(player->entity.cachedPath);
        player->entity.cachedPath = NULL;
    }
}
