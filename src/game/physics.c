#include "game/game.h"

#define BW_PLAYER_RADIUS 0.32f
#define BW_PLAYER_HEIGHT 1.75f

bool bw_player_overlaps_block(const BwPlayer *player, const BwBlock *block)
{
    const BwVec3 p = player->position;
    const BwVec3 b = block->position;
    const float r = BW_PLAYER_RADIUS;
    const float h = block->half;
    return p.x + r > b.x - h && p.x - r < b.x + h &&
           p.y + BW_PLAYER_HEIGHT > b.y - h && p.y < b.y + h &&
           p.z + r > b.z - h && p.z - r < b.z + h;
}

bool bw_player_collides(const BwGame *game, float x, float feet_y, float z)
{
    BwPlayer candidate = {0};
    candidate.position = (BwVec3){x, feet_y, z};
    if (feet_y < -0.001f) return true;
    for (int i = 0; i < game->block_count; ++i)
        if (bw_player_overlaps_block(&candidate, &game->blocks[i])) return true;
    return false;
}
