#include "game/game.h"
const DWORD bw_slot_colors[BW_INVENTORY_SLOTS]={
 D3DCOLOR_ARGB(255,90,205,155),D3DCOLOR_ARGB(255,90,160,235),
 D3DCOLOR_ARGB(255,220,170,75),D3DCOLOR_ARGB(255,190,100,210),
 D3DCOLOR_ARGB(255,190,195,205),D3DCOLOR_ARGB(255,210,100,90),
 D3DCOLOR_ARGB(255,100,190,210),D3DCOLOR_ARGB(255,145,175,100)
};
void bw_world_add_block(BwGame *g,BwVec3 p,float h,DWORD c)
{
 if(g->block_count>=BW_MAX_BLOCKS) return;
 BwBlock *b=&g->blocks[g->block_count++]; b->position=p;b->half=h;b->color=c;
}
void bw_world_init(BwGame *g)
{
 g->block_count=0;
 bw_world_add_block(g,(BwVec3){0.0f,0.5f,4.0f},0.5f,bw_slot_colors[0]);
 bw_world_add_block(g,(BwVec3){2.5f,0.5f,5.0f},0.5f,bw_slot_colors[1]);
 bw_world_add_block(g,(BwVec3){-2.0f,1.0f,5.0f},1.0f,bw_slot_colors[2]);
}
