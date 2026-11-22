#ifndef BW_GAME_H
#define BW_GAME_H
#include <xtl.h>
#include <d3d9.h>
#define BW_MAX_PLAYERS 4
#define BW_MAX_BLOCKS 256
#define BW_MAX_PROPS 64
#define BW_MAX_RULES 64
#define BW_INVENTORY_SLOTS 8
#define BW_PLAYER_RADIUS 0.32f
#define BW_PLAYER_HEIGHT 1.75f
#define BW_EYE_HEIGHT 1.55f
struct BwVec3 { float x,y,z; BwVec3():x(0),y(0),z(0){} BwVec3(float X,float Y,float Z):x(X),y(Y),z(Z){} };
struct BwBlock { BwVec3 position; float half; DWORD color; bool spike; BwBlock():half(0),color(0),spike(false){} BwBlock(BwVec3 p,float h,DWORD c):position(p),half(h),color(c),spike(false){} };
struct BwPlayer { BwVec3 position; float yaw,pitch,vertical_velocity; int selected_slot,selected_weapon; float sword_durability,blaster_durability,weapon_anim; bool has_sword,has_blaster,grounded,was_grounded,died_event; int hotbar_page,selected_item; };
struct BwPickup { BwVec3 position; int type; bool active; BwPickup():type(0),active(false){} };
struct BwNpc { BwVec3 position,spawn_position; float yaw,cooldown,anim,vertical_velocity; int type; DWORD color; bool active,was_shot,was_falling; BwNpc():yaw(0),cooldown(0),anim(0),vertical_velocity(0),type(0),color(0),active(false),was_shot(false),was_falling(false){} };
struct BwProp { BwVec3 position,spawn_position,velocity; float half; DWORD color; bool active,was_falling; BwProp():half(0.5f),color(0),active(false),was_falling(false){} };
enum BwRuleOwner { BW_OWNER_NPC=0,BW_OWNER_PROP=1,BW_OWNER_BLOCK=2,BW_OWNER_PICKUP=3,BW_OWNER_PLAYER=4 };
enum BwRuleCondition { BW_IF_SEE_PLAYER=0,BW_IF_BUMP_GREEN=1,BW_IF_SHOT=2,BW_IF_TOUCH_PLAYER=3,BW_IF_TIMER=4,BW_IF_NEAR_ITEM=5,BW_IF_ON_DEATH=6,BW_IF_ON_FALL=7,BW_IF_ON_STOP_FALLING=8,BW_IF_ON_LAND=9,BW_IF_ON_JUMP=10,BW_IF_ON_USE=11 };
enum BwRuleAction { BW_DO_MOVE_TOWARD=0,BW_DO_MOVE_BACKWARD=1,BW_DO_TURN=2,BW_DO_GREEN=3,BW_DO_DISAPPEAR=4,BW_DO_RESPAWN=5,BW_DO_JUMP=6,BW_DO_SHOOT=7,BW_DO_MOVE_CAMERA=8,BW_DO_SUMMON=9,BW_DO_PLACE_BLOCK=10,BW_DO_CUSTOM_WEAPON=11 };
struct BwRule { int owner_type,owner,condition,action,parameter,target_type,target_index; bool active,was_triggered; float timer; BwRule():owner_type(0),owner(0),condition(0),action(0),parameter(0),target_type(0),target_index(0),active(false),was_triggered(false),timer(0.0f){} };
struct BwPad { XINPUT_GAMEPAD current,previous; bool connected,joined,pressed_a,pressed_b,pressed_x,pressed_y,pressed_back,pressed_start,pressed_left,pressed_right,pressed_up,pressed_down,pressed_lthumb,pressed_rthumb,pressed_lb,pressed_rb; float lx,ly,rx,ry,lt,rt; };
struct BwProjectile { BwVec3 position,velocity; int owner; float life; bool active,npc_owned; BwProjectile():owner(0),life(0),active(false),npc_owned(false){} };
struct BwGame { BwPlayer players[BW_MAX_PLAYERS]; BwPad pads[BW_MAX_PLAYERS]; int active_slots[BW_MAX_PLAYERS],player_count; BwBlock blocks[BW_MAX_BLOCKS]; int block_count; BwProjectile projectiles[64]; BwPickup pickups[32]; BwNpc npcs[8]; int pickup_count,npc_count; BwProp props[BW_MAX_PROPS]; int prop_count; BwRule rules[BW_MAX_RULES]; int rule_count; int script_npc,script_condition,script_action,script_owner_type,script_owner,script_target_type,script_target_index,script_summon_type,selected_object_type,selected_object_index,save_status; bool editor_mode,show_grid,split_screen,paused,automatic_render_distance,running,placing_spikes,script_editor; int menu_selection; float render_distance,quality_scale,fps_average,player_speed,jump_height,gravity; BwVec3 spawn_point; };
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
void bw_players_update(BwGame*,float);
bool bw_editor_select_object(BwGame*,int);
void bw_quality_update(BwGame*,float);
bool bw_renderer_open(void);
void bw_renderer_draw(const BwGame*);
void bw_renderer_close(void);
#endif
