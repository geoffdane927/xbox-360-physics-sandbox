#include "game/game.h"
#include <math.h>
static void reset_player(BwPlayer *p,int i,bool editor){p->position=BwVec3((i-1.5f)*1.2f,editor?2.5f:BW_EYE_HEIGHT,-8.0f);p->yaw=p->pitch=p->vertical_velocity=0.0f;p->selected_slot=0;p->selected_weapon=0;p->grounded=true;}
static BwVec3 editor_place_position(const BwPlayer *p)
{
 float fx=sinf(p->yaw),fz=cosf(p->yaw);
 BwVec3 q(p->position.x+fx*4.0f,p->position.y-0.5f,p->position.z+fz*4.0f);
 q.x=floorf(q.x+0.5f);q.y=floorf(q.y+0.5f)+0.5f;q.z=floorf(q.z+0.5f);
 return q;
}
static void place_block(BwGame *g,BwPlayer *p)
{
 float fx=sinf(p->yaw),fz=cosf(p->yaw);BwVec3 pos(p->position.x+fx*3.0f,0.5f,p->position.z+fz*3.0f);
 pos.x=floorf(pos.x+0.5f);pos.z=floorf(pos.z+0.5f);BwBlock candidate(pos,0.5f,0);
 if(!bw_player_overlaps_block(p->position.x,p->position.y-BW_EYE_HEIGHT,p->position.z,&candidate))bw_world_add_block(g,pos,0.5f,bw_slot_colors[p->selected_slot]);
}
static void update_one(BwGame *g,int i,float dt)
{
 BwPlayer *p=&g->players[i];BwPad *pad=&g->pads[i];float speed=g->editor_mode?8.0f:g->player_speed;
 if(pad->pressed_left)p->selected_slot=(p->selected_slot+BW_INVENTORY_SLOTS-1)%BW_INVENTORY_SLOTS;
 if(pad->pressed_right)p->selected_slot=(p->selected_slot+1)%BW_INVENTORY_SLOTS;
 if(pad->pressed_lb)p->selected_weapon=(p->selected_weapon+1)%2;
 if(pad->pressed_rb)p->selected_weapon=(p->selected_weapon+1)%2;
 p->yaw+=pad->rx*1.8f*dt;p->pitch=bw_clamp(p->pitch+pad->ry*1.8f*dt,-1.45f,1.45f);
 float fx=sinf(p->yaw),fz=cosf(p->yaw),rx=cosf(p->yaw),rz=-sinf(p->yaw);
 float dx=(fx*pad->ly+rx*pad->lx)*speed*dt,dz=(fz*pad->ly+rz*pad->lx)*speed*dt;
 if(g->editor_mode){
  p->position.x+=dx;p->position.z+=dz;p->position.y+=(pad->rt-pad->lt)*speed*dt;
  if(pad->pressed_lthumb)g->placing_spikes=!g->placing_spikes;
  if(pad->pressed_a){
   BwVec3 q=editor_place_position(p);
   if(g->placing_spikes)bw_world_add_spike(g,q);
   else bw_world_add_block(g,q,0.5f,bw_slot_colors[p->selected_slot]);
  }
  if(pad->pressed_x&&g->block_count>0)--g->block_count;if(pad->pressed_b)g->show_grid=!g->show_grid;
 }else{
  float feet=p->position.y-BW_EYE_HEIGHT;
  if(bw_player_touches_spike(g,p->position.x,feet,p->position.z)){
   p->position=g->spawn_point;p->vertical_velocity=0.0f;p->grounded=true;feet=p->position.y-BW_EYE_HEIGHT;
  }
  float nx=p->position.x+dx,nz=p->position.z+dz;
  if(!bw_player_collides(g,nx,feet,p->position.z))p->position.x=nx;if(!bw_player_collides(g,p->position.x,feet,nz))p->position.z=nz;
  if(pad->pressed_a&&p->grounded){p->vertical_velocity=sqrtf(2.0f*g->gravity*g->jump_height);p->grounded=false;}
  p->vertical_velocity-=g->gravity*dt;float next=feet+p->vertical_velocity*dt;bool landed=false;
  if(p->vertical_velocity<=0.0f){
   float landing=(feet>=0.0f&&next<=0.0f)?0.0f:-100000.0f;
   for(int b=0;b<g->block_count;++b){const BwBlock *block=&g->blocks[b];if(block->spike)continue;float top=block->position.y+block->half;
    bool overlap=p->position.x+BW_PLAYER_RADIUS>block->position.x-block->half&&p->position.x-BW_PLAYER_RADIUS<block->position.x+block->half&&p->position.z+BW_PLAYER_RADIUS>block->position.z-block->half&&p->position.z-BW_PLAYER_RADIUS<block->position.z+block->half;
    if(overlap&&feet>=top-0.02f&&next<=top&&top>landing)landing=top;
   }
   if(landing>-99999.0f){next=landing;p->vertical_velocity=0.0f;p->grounded=true;landed=true;}
  }
  if(!landed){p->grounded=false;if(p->vertical_velocity>0.0f&&bw_player_collides(g,p->position.x,next,p->position.z)){next=feet;p->vertical_velocity=0.0f;}}
  p->position.y=next+BW_EYE_HEIGHT;
  if(bw_player_touches_spike(g,p->position.x,next,p->position.z)){
   p->position=g->spawn_point;p->vertical_velocity=0.0f;p->grounded=true;
  }
  if(pad->pressed_x){
   if(p->selected_weapon==0){
    float fx=sinf(p->yaw)*cosf(p->pitch),fy=sinf(p->pitch),fz=cosf(p->yaw)*cosf(p->pitch);
    for(int t=0;t<g->player_count;++t){int target=g->active_slots[t];if(target==i)continue;BwPlayer *other=&g->players[target];
     float ox=other->position.x-p->position.x,oy=other->position.y-p->position.y,oz=other->position.z-p->position.z;
     float along=ox*fx+oy*fy+oz*fz;float side2=ox*ox+oy*oy+oz*oz-along*along;
     if(along>0.0f&&along<=1.0f&&side2<0.55f&&fabsf(oy)<1.5f){other->position=g->spawn_point;other->vertical_velocity=0.0f;other->grounded=true;}
    }
   }else{
    for(int q=0;q<64;++q)if(!g->projectiles[q].active){
     BwProjectile *shot=&g->projectiles[q];float cp=cosf(p->pitch);
     shot->position=BwVec3(p->position.x+sinf(p->yaw)*cp*0.65f,p->position.y+sinf(p->pitch)*0.65f,p->position.z+cosf(p->yaw)*cp*0.65f);
     shot->velocity=BwVec3(sinf(p->yaw)*cp*28.0f,sinf(p->pitch)*28.0f,cosf(p->yaw)*cp*28.0f);shot->owner=i;shot->life=3.0f;shot->active=true;break;
    }
   }
  }
 }

}
static void update_projectiles(BwGame *g,float dt){
 for(int q=0;q<64;++q){BwProjectile *s=&g->projectiles[q];if(!s->active)continue;
  s->life-=dt;if(s->life<=0.0f){s->active=false;continue;}
  s->position.x+=s->velocity.x*dt;s->position.y+=s->velocity.y*dt;s->position.z+=s->velocity.z*dt;
  if(s->position.y<0.0f){s->active=false;continue;}
  for(int b=0;b<g->block_count;++b){const BwBlock *v=&g->blocks[b];if(s->position.x>v->position.x-v->half&&s->position.x<v->position.x+v->half&&s->position.y>v->position.y-v->half&&s->position.y<v->position.y+v->half&&s->position.z>v->position.z-v->half&&s->position.z<v->position.z+v->half){s->active=false;break;}}
  if(!s->active)continue;
  for(int t=0;t<g->player_count;++t){int target=g->active_slots[t];if(target==s->owner)continue;BwPlayer *p=&g->players[target];
   if(fabsf(s->position.x-p->position.x)<BW_PLAYER_RADIUS+0.12f&&fabsf(s->position.z-p->position.z)<BW_PLAYER_RADIUS+0.12f&&s->position.y>p->position.y-BW_EYE_HEIGHT&&s->position.y<p->position.y-BW_EYE_HEIGHT+BW_PLAYER_HEIGHT){p->position=g->spawn_point;p->vertical_velocity=0.0f;p->grounded=true;s->active=false;break;}
  }
 }
}
void bw_players_update(BwGame *g,float dt){for(int i=0;i<BW_MAX_PLAYERS;++i)if(g->pads[i].joined)update_one(g,i,dt);update_projectiles(g,dt);}
float bw_clamp(float v,float lo,float hi){return v<lo?lo:(v>hi?hi:v);}
void bw_quality_update(BwGame *g,float seconds){if(seconds<=0.0f)return;float fps=1.0f/seconds;g->fps_average=g->fps_average*0.90f+fps*0.10f;if(g->fps_average<48.0f&&g->quality_scale>0.55f)g->quality_scale-=0.05f;else if(g->fps_average>57.0f&&g->quality_scale<1.0f)g->quality_scale+=0.025f;g->quality_scale=bw_clamp(g->quality_scale,0.55f,1.0f);}
