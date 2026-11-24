#ifndef BW_GAME_H
#define BW_GAME_H
#include <xtl.h>
#include <d3d9.h>
#define BW_MAX_PLAYERS 4
#define BW_MAX_BLOCKS 256
#define BW_MAX_PROPS 64
#define BW_INVENTORY_SLOTS 8
#define BW_PLAYER_RADIUS 0.32f
#define BW_PLAYER_HEIGHT 1.75f
#define BW_EYE_HEIGHT 1.55f
struct BwVec3 { float x,y,z; BwVec3():x(0),y(0),z(0){} BwVec3(float X,float Y,float Z):x(X),y(Y),z(Z){} };
struct BwBlock { BwVec3 position; float half,friction; DWORD color; bool spike,ice; BwBlock():half(0),friction(0.82f),color(0),spike(false),ice(false){} BwBlock(BwVec3 p,float h,DWORD c):position(p),half(h),friction(0.82f),color(c),spike(false),ice(false){} };
struct BwPlayer { BwVec3 position; float yaw,pitch,vertical_velocity; int selected_slot,selected_weapon; float sword_durability,blaster_durability,weapon_anim; bool has_sword,has_blaster,grounded,was_grounded,died_event; int hotbar_page,selected_item,grabbed_prop; float grab_distance; };
struct BwPickup { BwVec3 position; int type; bool active; BwPickup():type(0),active(false){} };
struct BwNpc { BwVec3 position,spawn_position; float yaw,cooldown,anim,vertical_velocity; int type; DWORD color; bool active,was_shot,was_falling; BwNpc():yaw(0),cooldown(0),anim(0),vertical_velocity(0),type(0),color(0),active(false),was_shot(false),was_falling(false){} };
struct BwProp { BwVec3 position,spawn_position,velocity,previous_position; float half,mass,friction,gravity_scale; DWORD color; bool active,was_falling; BwProp():half(0.5f),mass(1.0f),friction(0.985f),gravity_scale(1.0f),color(0),active(false),was_falling(false){} };
#define BW_MAX_LINKS 128
struct BwLink { int a,b,type; float rest_length,phase; bool active; BwLink():a(-1),b(-1),type(0),rest_length(1.0f),phase(0),active(false){} };
struct BwPad { XINPUT_GAMEPAD current,previous; bool connected,joined,pressed_a,pressed_b,pressed_x,pressed_y,pressed_back,pressed_start,pressed_left,pressed_right,pressed_up,pressed_down,pressed_lthumb,pressed_rthumb,pressed_lb,pressed_rb; float lx,ly,rx,ry,lt,rt; };
struct BwProjectile { BwVec3 position,velocity; int owner; float life; bool active,npc_owned; BwProjectile():owner(0),life(0),active(false),npc_owned(false){} };
struct BwGame { BwPlayer players[BW_MAX_PLAYERS]; BwPad pads[BW_MAX_PLAYERS]; int active_slots[BW_MAX_PLAYERS],player_count; BwBlock blocks[BW_MAX_BLOCKS]; int block_count; BwProjectile projectiles[64]; BwPickup pickups[32]; BwNpc npcs[8]; int pickup_count,npc_count; BwProp props[BW_MAX_PROPS]; int prop_count; BwLink links[BW_MAX_LINKS]; int link_count,link_anchor_prop,link_anchor_tool; int selected_object_type,selected_object_index,save_status; bool editor_mode,show_grid,split_screen,paused,automatic_render_distance,running,placing_spikes; int menu_selection; float render_distance,quality_scale,fps_average,player_speed,jump_height,gravity,physics_gravity,physics_friction; BwVec3 spawn_point; };
extern const DWORD bw_slot_colors[BW_INVENTORY_SLOTS];
float bw_clamp(float,float,float);
void bw_input_poll(BwGame*);
void bw_world_init(BwGame*);
void bw_world_add_block(BwGame*,BwVec3,float,DWORD);
void bw_world_add_spike(BwGame*,BwVec3);
void bw_world_add_pickup(BwGame*,BwVec3,int);
void bw_world_add_npc(BwGame*,BwVec3,int);
void bw_world_add_prop(BwGame*,BwVec3);
bool bw_world_save(const BwGame*);
bool bw_world_load(BwGame*);
bool bw_player_touches_spike(const BwGame*,float,float,float);
bool bw_player_overlaps_block(float,float,float,const BwBlock*);
bool bw_player_collides(const BwGame*,float,float,float);
BwVec3 bw_editor_block_preview(const BwGame*,const BwPlayer*);
void bw_players_update(BwGame*,float);
void bw_quality_update(BwGame*,float);
bool bw_renderer_open(void);
void bw_renderer_draw(const BwGame*);
void bw_renderer_close(void);
#endif
