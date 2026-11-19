#ifndef BW_GAME_H
#define BW_GAME_H
#include <stdbool.h>
#include <stdint.h>
#include "xsdk/kernel.h"
#define BW_MAX_PLAYERS 4
#define BW_MAX_BLOCKS 256
#define BW_INVENTORY_SLOTS 8
typedef struct { float x, y, z; } BwVec3;
typedef struct { BwVec3 position; float half; uint32_t color; } BwBlock;
typedef struct {
    BwVec3 position;
    float yaw, pitch, vertical_velocity;
    int selected_slot;
    bool grounded;
} BwPlayer;
typedef struct {
    INPUT_STATE state;
    uint16_t previous_buttons;
    bool connected;
    bool pressed_a, pressed_b, pressed_x, pressed_back, pressed_start, pressed_guide;
    bool pressed_left, pressed_right;
    float lx, ly, rx, ry, lt, rt;
} BwPad;
typedef struct {
    BwPlayer players[BW_MAX_PLAYERS];
    BwPad pads[BW_MAX_PLAYERS];
    int active_slots[BW_MAX_PLAYERS];
    int player_count;
    BwBlock blocks[BW_MAX_BLOCKS];
    int block_count;
    bool editor_mode, show_grid;
    float render_distance, quality_scale;
} BwGame;
float bw_clampf(float value, float low, float high);
void bw_input_poll(BwGame *game);
void bw_world_init(BwGame *game);
void bw_world_add_block(BwGame *game, BwVec3 position, float half, uint32_t color);
bool bw_player_overlaps_block(const BwPlayer *player, const BwBlock *block);
bool bw_player_collides(const BwGame *game, float x, float feet_y, float z);
void bw_players_update(BwGame *game, float dt);
#endif
