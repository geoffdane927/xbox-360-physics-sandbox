#include "game/game.h"
const DWORD bw_slot_colors[BW_INVENTORY_SLOTS]={
 D3DCOLOR_ARGB(255,90,205,155),D3DCOLOR_ARGB(255,90,160,235),
 D3DCOLOR_ARGB(255,220,170,75),D3DCOLOR_ARGB(255,190,100,210),
 D3DCOLOR_ARGB(255,190,195,205),D3DCOLOR_ARGB(255,210,100,90),
 D3DCOLOR_ARGB(255,100,190,210),D3DCOLOR_ARGB(255,145,175,100)
};
void bw_world_add_block(BwGame *g,BwVec3 p,float h,DWORD c){if(g->block_count>=BW_MAX_BLOCKS)return;g->blocks[g->block_count++]=BwBlock(p,h,c);}
void bw_world_add_pickup(BwGame *g,BwVec3 p,int type){if(g->pickup_count>=32)return;BwPickup *v=&g->pickups[g->pickup_count++];v->position=p;v->type=type;v->active=true;}
void bw_world_add_spike(BwGame *g,BwVec3 p){
 if(g->block_count>=BW_MAX_BLOCKS)return;
 BwBlock b(p,0.5f,D3DCOLOR_ARGB(255,190,65,65));b.spike=true;
 g->blocks[g->block_count++]=b;
}
void bw_world_init(BwGame *g)
{
 g->block_count=0;g->pickup_count=0;g->npc_count=0;g->placing_spikes=false;
 bw_world_add_pickup(g,BwVec3(3.0f,0.6f,-3.0f),1);bw_world_add_pickup(g,BwVec3(-3.0f,0.6f,-3.0f),2);
 g->npcs[0].position=BwVec3(4,0,2);g->npcs[0].type=0;g->npcs[0].active=true;g->npcs[1].position=BwVec3(-4,0,3);g->npcs[1].type=1;g->npcs[1].active=true;g->npcs[2].position=BwVec3(0,0,6);g->npcs[2].type=2;g->npcs[2].active=true;g->npc_count=3;
}
