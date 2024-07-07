// player.c
#include "player.h"

void InitPlayer(Player* player, float startX, float startY, float speed) {
    InitEntity(&player->entity, startX, startY, speed);
}
