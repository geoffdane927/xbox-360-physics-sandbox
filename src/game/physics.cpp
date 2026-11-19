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
  if(bw_player_overlaps_block(x,feet,z,&g->blocks[i])) return true;
 return false;
}
