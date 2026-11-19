#include "game/game.h"
#include <math.h>

#define BW_PI 3.14159265358979323846f
#define BW_PLAYER_RADIUS 0.32f
#define BW_PLAYER_HEIGHT 1.75f
#define BW_GRAVITY 18.0f
#define BW_JUMP_SPEED 6.0f

float bw_clampf(float value, float low, float high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static const uint32_t slot_colors[BW_INVENTORY_SLOTS] = {
    0xFF5ACD9B, 0xFF5AA0EB, 0xFFDCAA4B, 0xFFBE64D2,
    0xFFBEC3CD, 0xFFD2645A, 0xFF64BED2, 0xFF91AF64
};

static void reset_player(BwPlayer *player, int index)
{
    player->position = (BwVec3){((float)index - 1.5f) * 1.2f, 0.0f, -8.0f};
    player->yaw = 0.0f;
    player->pitch = 0.0f;
    player->vertical_velocity = 0.0f;
    player->selected_slot = 0;
    player->grounded = true;
}

static void place_player_block(BwGame *game, BwPlayer *player)
{
    const float fx = sinf(player->yaw), fz = cosf(player->yaw);
    BwVec3 pos = {player->position.x + fx * 3.0f, 0.5f, player->position.z + fz * 3.0f};
    pos.x = floorf(pos.x + 0.5f);
    pos.z = floorf(pos.z + 0.5f);
    BwBlock candidate = {pos, 0.5f, 0};
    if (!bw_player_overlaps_block(player, &candidate))
        bw_world_add_block(game, pos, 0.5f, slot_colors[player->selected_slot]);
}

static void update_one(BwGame *game, int index, float dt)
{
    BwPlayer *p = &game->players[index];
    BwPad *pad = &game->pads[index];
    const float speed = game->editor_mode ? 8.0f : 5.0f;
    p->yaw += pad->rx * 1.8f * dt;
    p->pitch = bw_clampf(p->pitch - pad->ry * 1.8f * dt, -1.45f, 1.45f);
    const float fx = sinf(p->yaw), fz = cosf(p->yaw);
    const float rx = cosf(p->yaw), rz = -sinf(p->yaw);
    const float dx = (fx * pad->ly + rx * pad->lx) * speed * dt;
    const float dz = (fz * pad->ly + rz * pad->lx) * speed * dt;

    if (game->editor_mode) {
        p->position.x += dx;
        p->position.z += dz;
        p->position.y += (pad->rt - pad->lt) * speed * dt;
        if (pad->pressed_a) {
            BwVec3 pos = {p->position.x + fx * 4.0f, p->position.y - 0.5f, p->position.z + fz * 4.0f};
            pos.x = floorf(pos.x + 0.5f); pos.y = floorf(pos.y + 0.5f); pos.z = floorf(pos.z + 0.5f);
            bw_world_add_block(game, pos, 0.5f, slot_colors[p->selected_slot]);
        }
        if (pad->pressed_x && game->block_count > 0) --game->block_count;
        if (pad->pressed_b) game->show_grid = !game->show_grid;
    } else {
        const float next_x = p->position.x + dx;
        const float next_z = p->position.z + dz;
        if (!bw_player_collides(game, next_x, p->position.y, p->position.z)) p->position.x = next_x;
        if (!bw_player_collides(game, p->position.x, p->position.y, next_z)) p->position.z = next_z;
        if (pad->pressed_a && p->grounded) {
            p->vertical_velocity = BW_JUMP_SPEED;
            p->grounded = false;
        }
        p->vertical_velocity -= BW_GRAVITY * dt;
        float next_y = p->position.y + p->vertical_velocity * dt;
        if (p->vertical_velocity <= 0.0f) {
            float landing = p->position.y >= 0.0f && next_y <= 0.0f ? 0.0f : -100000.0f;
            for (int b = 0; b < game->block_count; ++b) {
                const BwBlock *block = &game->blocks[b];
                const float top = block->position.y + block->half;
                const bool overlap = p->position.x + BW_PLAYER_RADIUS > block->position.x - block->half &&
                    p->position.x - BW_PLAYER_RADIUS < block->position.x + block->half &&
                    p->position.z + BW_PLAYER_RADIUS > block->position.z - block->half &&
                    p->position.z - BW_PLAYER_RADIUS < block->position.z + block->half;
                if (overlap && p->position.y >= top - 0.02f && next_y <= top && top > landing) landing = top;
            }
            if (landing > -99999.0f) {
                next_y = landing; p->vertical_velocity = 0.0f; p->grounded = true;
            } else p->grounded = false;
        } else if (bw_player_collides(game, p->position.x, next_y, p->position.z)) {
            next_y = p->position.y; p->vertical_velocity = 0.0f;
        }
        p->position.y = next_y;
        if (pad->pressed_left) p->selected_slot = (p->selected_slot + BW_INVENTORY_SLOTS - 1) % BW_INVENTORY_SLOTS;
        if (pad->pressed_right) p->selected_slot = (p->selected_slot + 1) % BW_INVENTORY_SLOTS;
        if (pad->pressed_x) place_player_block(game, p);
    }
    if (pad->pressed_start) reset_player(p, index);
}

void bw_players_update(BwGame *game, float dt)
{
    for (int i = 0; i < BW_MAX_PLAYERS; ++i)
        if (game->pads[i].connected || i == 0) update_one(game, i, dt);
}
