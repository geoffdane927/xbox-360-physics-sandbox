#ifndef BW_GAME_H
#define BW_GAME_H
#include <xtl.h>
#include <d3d9.h>
#define BW_MAX_PLAYERS 4
#define BW_MAX_BLOCKS 256
#define BW_INVENTORY_SLOTS 8
#define BW_PLAYER_RADIUS 0.32f
#define BW_PLAYER_HEIGHT 1.75f
#define BW_EYE_HEIGHT 1.55f
struct BwVec3 {
 float x,y,z;
 BwVec3():x(0),y(0),z(0){}
 BwVec3(float X,float Y,float Z):x(X),y(Y),z(Z){}
};
struct BwBlock {
 BwVec3 position; float half; DWORD color; bool spike;
 BwBlock():half(0),color(0),spike(false){}
 BwBlock(BwVec3 p,float h,DWORD c):position(p),half(h),color(c),spike(false){}
};
struct BwPlayer { BwVec3 position; float yaw,pitch,vertical_velocity; int selected_slot; bool grounded; };
struct BwPad { XINPUT_GAMEPAD current,previous; bool connected,pressed_a,pressed_b,pressed_x,pressed_y,pressed_back,pressed_start,pressed_left,pressed_right,pressed_up,pressed_down,pressed_lthumb; float lx,ly,rx,ry,lt,rt; };
struct BwGame { BwPlayer players[BW_MAX_PLAYERS]; BwPad pads[BW_MAX_PLAYERS]; int active_slots[BW_MAX_PLAYERS],player_count; BwBlock blocks[BW_MAX_BLOCKS]; int block_count; bool editor_mode,show_grid,split_screen,paused,automatic_render_distance,running,placing_spikes; int menu_selection; float render_distance,quality_scale,fps_average; float player_speed,jump_height,gravity; BwVec3 spawn_point; };
extern const DWORD bw_slot_colors[BW_INVENTORY_SLOTS];
float bw_clamp(float,float,float);
void bw_input_poll(BwGame*);
void bw_world_init(BwGame*);
void bw_world_add_block(BwGame*,BwVec3,float,DWORD);
void bw_world_add_spike(BwGame*,BwVec3);
bool bw_player_touches_spike(const BwGame*,float,float,float);
bool bw_player_overlaps_block(float,float,float,const BwBlock*);
bool bw_player_collides(const BwGame*,float,float,float);
void bw_players_update(BwGame*,float);
void bw_quality_update(BwGame*,float);
bool bw_renderer_open(void);
void bw_renderer_draw(const BwGame*);
void bw_renderer_close(void);
#endif
