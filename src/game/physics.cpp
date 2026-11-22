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

void bw_physics_update(BwGame *g,float dt){for(int i=0;i<g->block_count;++i){BwBlock *b=&g->blocks[i];if(!b->movable||b->spike)continue;b->velocity.y-=g->gravity*dt;b->position.x+=b->velocity.x*dt;b->position.z+=b->velocity.z*dt;float y=b->position.y+b->velocity.y*dt,ground=b->half;for(int j=0;j<g->block_count;++j){if(i==j)continue;BwBlock *o=&g->blocks[j];if(o->spike)continue;bool x=b->position.x+b->half>o->position.x-o->half&&b->position.x-b->half<o->position.x+o->half;bool z=b->position.z+b->half>o->position.z-o->half&&b->position.z-b->half<o->position.z+o->half;if(x&&z&&b->position.y>=o->position.y+o->half-.03f&&y-b->half<o->position.y+o->half&&b->velocity.y<0&&o->position.y+o->half>ground)ground=o->position.y+o->half;}if(y<ground+b->half){y=ground+b->half;b->velocity.y=0;}b->position.y=y;b->velocity.x*=.94f;b->velocity.z*=.94f;for(int p=0;p<g->player_count;++p){BwPlayer *pl=&g->players[g->active_slots[p]];float dx=b->position.x-pl->position.x,dz=b->position.z-pl->position.z;if(fabsf(dx)<b->half+BW_PLAYER_RADIUS&&fabsf(dz)<b->half+BW_PLAYER_RADIUS&&pl->position.y-BW_EYE_HEIGHT<b->position.y+b->half&&pl->position.y-BW_EYE_HEIGHT+BW_PLAYER_HEIGHT>b->position.y-b->half){float len=sqrtf(dx*dx+dz*dz);if(len<.01f)len=.01f;b->velocity.x+=dx/len*.65f;b->velocity.z+=dz/len*.65f;}}}}
