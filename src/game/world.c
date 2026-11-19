#include "game/game.h"
static const uint32_t colors[BW_INVENTORY_SLOTS] = {
    0xFF5ACD9B, 0xFF5AA0EB, 0xFFDCAA4B, 0xFFBE64D2,
    0xFFBEC3CD, 0xFFD2645A, 0xFF64BED2, 0xFF91AF64
};
void bw_world_add_block(BwGame *game, BwVec3 position, float half, uint32_t color)
{
    if (game->block_count >= BW_MAX_BLOCKS) return;
    BwBlock *block = &game->blocks[game->block_count++];
    block->position = position; block->half = half; block->color = color;
}
void bw_world_init(BwGame *game)
{
    game->block_count = 0;
    bw_world_add_block(game, (BwVec3){0.0f,-0.10f,0.0f}, 40.0f, 0xFF303B48);
    bw_world_add_block(game, (BwVec3){0.0f,0.5f,4.0f}, 0.5f, colors[0]);
    bw_world_add_block(game, (BwVec3){2.5f,0.5f,5.0f}, 0.5f, colors[1]);
    bw_world_add_block(game, (BwVec3){-2.0f,1.0f,5.0f}, 1.0f, colors[2]);
}
