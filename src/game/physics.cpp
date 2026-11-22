#include "game/game.h"
bool bw_player_overlaps_block(float x,float feet,float z,const BwBlock *b)
{
 float r=BW_PLAYER_RADIUS,h=b->half; BwVec3 p=b->position;
 return x+r>p.x-h && x-r<p.x+h && feet+BW_PLAYER_HEIGHT>p.y-h &&
        feet<p.y+h && z+r>p.z-h && z-r<p.z+h;
}
bool bw_player_collides(const BwGame *g,float x,float feet,float z)
{
 if(feet< -0.001f) return true;
 for(int i=0;i<g->block_count;++i)
  if(!g->blocks[i].spike&&bw_player_overlaps_block(x,feet,z,&g->blocks[i])) return true;
 return false;
}

bool bw_player_touches_spike(const BwGame *g,float x,float feet,float z)
{
 for(int i=0;i<g->block_count;++i){
  const BwBlock *b=&g->blocks[i];if(!b->spike)continue;
  float base=b->position.y-b->half,top=base+1.0f;
  if(x+BW_PLAYER_RADIUS>b->position.x-b->half&&x-BW_PLAYER_RADIUS<b->position.x+b->half&&
     z+BW_PLAYER_RADIUS>b->position.z-b->half&&z-BW_PLAYER_RADIUS<b->position.z+b->half&&
     feet+BW_PLAYER_HEIGHT>base&&feet<top)return true;
 }
 return false;
}
