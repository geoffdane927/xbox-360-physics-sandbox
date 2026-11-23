#include "game/game.h"
#include <math.h>
static void reset_player(BwPlayer *p,int i,bool editor){p->position=BwVec3((i-1.5f)*1.2f,editor?2.5f:BW_EYE_HEIGHT,-8.0f);p->yaw=p->pitch=p->vertical_velocity=0.0f;p->selected_slot=0;p->selected_weapon=0;p->has_sword=false;p->has_blaster=false;p->sword_durability=100.0f;p->blaster_durability=100.0f;p->weapon_anim=0.0f;p->grounded=true;p->was_grounded=true;p->died_event=false;p->hotbar_page=0;p->selected_item=0;}
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
static int editor_pick_prop(const BwGame *g,const BwPlayer *p){float cp=cosf(p->pitch),dx=sinf(p->yaw)*cp,dy=sinf(p->pitch),dz=cosf(p->yaw)*cp,best=9999.0f;int found=-1;for(int i=0;i<g->prop_count;++i){const BwProp *o=&g->props[i];if(!o->active)continue;float vx=o->position.x-p->position.x,vy=o->position.y-p->position.y,vz=o->position.z-p->position.z,along=vx*dx+vy*dy+vz*dz;if(along<0||along>16||along>=best)continue;float ex=vx-dx*along,ey=vy-dy*along,ez=vz-dz*along;if(ex*ex+ey*ey+ez*ez<1){best=along;found=i;}}return found;}
static void editor_use_physics_tool(BwGame *g,BwPlayer *p,int tool){int target=editor_pick_prop(g,p);if(target<0)return;if(g->link_anchor_prop<0||g->link_anchor_prop>=g->prop_count||!g->props[g->link_anchor_prop].active||g->link_anchor_tool!=tool){g->link_anchor_prop=target;g->link_anchor_tool=tool;return;}if(target==g->link_anchor_prop||g->link_count>=BW_MAX_LINKS)return;BwVec3 a=g->props[g->link_anchor_prop].position,b=g->props[target].position;float x=b.x-a.x,y=b.y-a.y,z=b.z-a.z;BwLink *l=&g->links[g->link_count++];l->a=g->link_anchor_prop;l->b=target;l->type=tool;l->rest_length=sqrtf(x*x+y*y+z*z);l->phase=0;l->active=true;g->link_anchor_prop=-1;g->link_anchor_tool=-1;}
static void update_one(BwGame *g,int i,float dt)
{
 BwPlayer *p=&g->players[i];BwPad *pad=&g->pads[i];float speed=g->editor_mode?8.0f:g->player_speed;
 if(pad->pressed_left)p->selected_slot=(p->selected_slot+BW_INVENTORY_SLOTS-1)%BW_INVENTORY_SLOTS;
 if(pad->pressed_right)p->selected_slot=(p->selected_slot+1)%BW_INVENTORY_SLOTS;
 if(g->editor_mode&&pad->pressed_lthumb)p->hotbar_page=(p->hotbar_page+1)%3;if(g->editor_mode&&pad->pressed_y)p->hotbar_page=(p->hotbar_page+1)%3;
 if(g->editor_mode){if(pad->pressed_lb){if(p->hotbar_page==0)p->selected_slot=(p->selected_slot+7)%8;else p->selected_item=(p->selected_item+7)%8;}if(pad->pressed_rb){if(p->hotbar_page==0)p->selected_slot=(p->selected_slot+1)%8;else p->selected_item=(p->selected_item+1)%8;}}
 else{if(pad->pressed_lb)p->selected_weapon=(p->selected_weapon+4)%5;if(pad->pressed_rb)p->selected_weapon=(p->selected_weapon+1)%5;}
 if(p->weapon_anim>0.0f)p->weapon_anim-=dt;
 p->yaw+=pad->rx*1.8f*dt;p->pitch=bw_clamp(p->pitch+pad->ry*1.8f*dt,-1.45f,1.45f);
 float fx=sinf(p->yaw),fz=cosf(p->yaw),rx=cosf(p->yaw),rz=-sinf(p->yaw);
 float dx=(fx*pad->ly+rx*pad->lx)*speed*dt,dz=(fz*pad->ly+rz*pad->lx)*speed*dt;
 if(g->editor_mode){
  p->position.x+=dx;p->position.z+=dz;p->position.y+=(pad->rt-pad->lt)*speed*dt;
  if(pad->pressed_a){if(p->hotbar_page==0){BwVec3 q=editor_place_position(p);bw_world_add_block(g,q,0.5f,bw_slot_colors[p->selected_slot]);}
   else{BwVec3 q=editor_place_position(p);if(p->hotbar_page==1){if(p->selected_item==0)bw_world_add_spike(g,q);else if(p->selected_item==1)bw_world_add_pickup(g,BwVec3(q.x,0.6f,q.z),1);else if(p->selected_item==2)bw_world_add_pickup(g,BwVec3(q.x,0.6f,q.z),2);else if(p->selected_item==3)g->spawn_point=BwVec3(q.x,BW_EYE_HEIGHT,q.z);else if(p->selected_item==4)bw_world_add_npc(g,BwVec3(q.x,0,q.z),0);else if(p->selected_item==5)bw_world_add_npc(g,BwVec3(q.x,0,q.z),1);else if(p->selected_item==6)bw_world_add_npc(g,BwVec3(q.x,0,q.z),2);else bw_world_add_prop(g,BwVec3(q.x,q.y,q.z));}else{if(p->selected_item<=3)editor_use_physics_tool(g,p,p->selected_item);else if(p->selected_item==4){bw_world_add_block(g,q,.5f,D3DCOLOR_ARGB(255,145,220,245));if(g->block_count>0){g->blocks[g->block_count-1].ice=true;g->blocks[g->block_count-1].friction=.03f;}}else if(p->selected_item==5)bw_world_add_prop(g,BwVec3(q.x,q.y,q.z));}}}
  if(pad->pressed_x){if(g->link_count>0)--g->link_count;else if(g->block_count>0)--g->block_count;else if(g->prop_count>0)g->props[g->prop_count-1].active=false;g->link_anchor_prop=-1;g->link_anchor_tool=-1;}if(pad->pressed_b)g->show_grid=!g->show_grid;
 }else{
  float feet=p->position.y-BW_EYE_HEIGHT;
  if(bw_player_touches_spike(g,p->position.x,feet,p->position.z)){
   p->position=g->spawn_point;p->vertical_velocity=0.0f;p->grounded=true;p->died_event=true;feet=p->position.y-BW_EYE_HEIGHT;
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
   p->position=g->spawn_point;p->vertical_velocity=0.0f;p->grounded=true;p->died_event=true;
  }
  for(int k=0;k<g->prop_count;++k){BwProp *o=&g->props[k];if(!o->active)continue;float ox=o->position.x-p->position.x,oz=o->position.z-p->position.z,od=sqrtf(ox*ox+oz*oz);if(od<0.9f&&od>0.01f){o->previous_position.x-=ox/od*3.5f*dt;o->previous_position.z-=oz/od*3.5f*dt;}}
  for(int k=0;k<g->pickup_count;++k){BwPickup *it=&g->pickups[k];if(it->active&&fabsf(it->position.x-p->position.x)<0.8f&&fabsf(it->position.z-p->position.z)<0.8f){if(it->type==1){p->has_blaster=true;p->blaster_durability=100.0f;}if(it->type==2){p->has_sword=true;p->sword_durability=100.0f;}it->active=false;}}
  if(pad->pressed_x&&p->selected_weapon==2&&p->has_sword&&p->sword_durability>0.0f){p->weapon_anim=0.28f;p->sword_durability-=10.0f;if(p->sword_durability<=0)p->has_sword=false;float fx=sinf(p->yaw)*cosf(p->pitch),fz=cosf(p->yaw)*cosf(p->pitch);for(int t=0;t<g->player_count;++t){int target=g->active_slots[t];if(target==i)continue;BwPlayer *o=&g->players[target];float dx=o->position.x-p->position.x,dz=o->position.z-p->position.z,along=dx*fx+dz*fz,side=fabsf(dx*fz-dz*fx);if(along>0&&along<1.0f&&side<0.7f){o->position=g->spawn_point;o->vertical_velocity=0;o->grounded=true;o->died_event=true;}}for(int n=0;n<g->npc_count;++n){BwNpc *o=&g->npcs[n];float dx=o->position.x-p->position.x,dz=o->position.z-p->position.z,along=dx*fx+dz*fz,side=fabsf(dx*fz-dz*fx);if(o->active&&o->type>0&&along>0&&along<1.0f&&side<0.7f)o->active=false;}}
  if(pad->pressed_x&&p->selected_weapon==1&&p->has_blaster&&p->blaster_durability>0.0f){p->weapon_anim=0.18f;p->blaster_durability-=5.0f;if(p->blaster_durability<=0)p->has_blaster=false;for(int q=0;q<64;++q)if(!g->projectiles[q].active){BwProjectile *s=&g->projectiles[q];float cp=cosf(p->pitch);s->position=BwVec3(p->position.x+sinf(p->yaw)*cp*.65f,p->position.y+sinf(p->pitch)*.65f,p->position.z+cosf(p->yaw)*cp*.65f);s->velocity=BwVec3(sinf(p->yaw)*cp*28.0f,sinf(p->pitch)*28.0f,cosf(p->yaw)*cp*28.0f);s->owner=i;s->life=3.0f;s->active=true;s->npc_owned=false;break;}} }

}
static bool npc_blocked(const BwGame *g,float x,float z){for(int b=0;b<g->block_count;++b){const BwBlock *v=&g->blocks[b];if(v->color&&!v->spike&&x>v->position.x-v->half-.32f&&x<v->position.x+v->half+.32f&&z>v->position.z-v->half-.32f&&z<v->position.z+v->half+.32f)return true;}return false;}
static void update_npcs(BwGame *g,float dt){for(int n=0;n<g->npc_count;++n){BwNpc *a=&g->npcs[n];if(!a->active||g->player_count<1)continue;BwPlayer *pl=&g->players[g->active_slots[0]];float dx=pl->position.x-a->position.x,dz=pl->position.z-a->position.z,d=sqrtf(dx*dx+dz*dz);a->cooldown-=dt;if(d>.01f)a->yaw=(float)atan2(dx,dz);if(a->type==0){if(d>2.5f&&d<20){float nx=a->position.x+dx/d*1.2f*dt,nz=a->position.z+dz/d*1.2f*dt;if(!npc_blocked(g,nx,nz)){a->position.x=nx;a->position.z=nz;}}}else if(a->type==1&&d>.85f){float nx=a->position.x+dx/d*1.8f*dt,nz=a->position.z+dz/d*1.8f*dt;if(npc_blocked(g,nx,nz)&&a->position.y<=.01f)a->vertical_velocity=5.5f;else if(!npc_blocked(g,nx,nz)){a->position.x=nx;a->position.z=nz;}}else if(a->type==1&&a->cooldown<=0){pl->position=g->spawn_point;pl->vertical_velocity=0;pl->grounded=true;a->cooldown=1.0f;}else if(a->type==2&&a->cooldown<=0&&d<18){for(int q=0;q<64;++q)if(!g->projectiles[q].active){BwProjectile *s=&g->projectiles[q];s->position=BwVec3(a->position.x,1.2f,a->position.z);s->velocity=BwVec3(dx/d*18,0,dz/d*18);s->owner=-1;s->life=2.5f;s->active=true;s->npc_owned=true;a->cooldown=1.5f;break;}}if(a->vertical_velocity!=0||a->position.y>0){a->vertical_velocity-=9.8f*dt;a->position.y+=a->vertical_velocity*dt;if(a->position.y<0){a->position.y=0;a->vertical_velocity=0;}}}}
static void update_props(BwGame *g,float dt){if(dt<=0)return;float dt2=dt*dt;for(int i=0;i<g->prop_count;++i){BwProp *o=&g->props[i];if(!o->active)continue;BwVec3 old=o->position;float damp=bw_clamp(o->friction*g->physics_friction,0,.9999f);float vx=(o->position.x-o->previous_position.x)*damp,vy=(o->position.y-o->previous_position.y)*damp,vz=(o->position.z-o->previous_position.z)*damp;o->position.x+=vx;o->position.y+=vy-g->physics_gravity*o->gravity_scale*dt2;o->position.z+=vz;o->previous_position=old;}
for(int it=0;it<7;++it){for(int k=0;k<g->link_count;++k){BwLink *l=&g->links[k];if(!l->active||l->a<0||l->b<0||l->a>=g->prop_count||l->b>=g->prop_count)continue;BwProp *a=&g->props[l->a],*b=&g->props[l->b];if(!a->active||!b->active)continue;if(l->type==3&&it==0)l->phase+=dt*2.5f;float target=l->rest_length+(l->type==3?sinf(l->phase)*.65f:0);float dx=b->position.x-a->position.x,dy=b->position.y-a->position.y,dz=b->position.z-a->position.z,d=sqrtf(dx*dx+dy*dy+dz*dz);if(d<.0001f||(l->type==1&&d<=target))continue;float wa=1.0f/bw_clamp(a->mass,.1f,100),wb=1.0f/bw_clamp(b->mass,.1f,100),stiff=l->type==2?.65f:1.0f,c=((d-target)/d)*stiff/(wa+wb),cx=dx*c,cy=dy*c,cz=dz*c;a->position.x+=cx*wa;a->position.y+=cy*wa;a->position.z+=cz*wa;b->position.x-=cx*wb;b->position.y-=cy*wb;b->position.z-=cz*wb;}
for(int i=0;i<g->prop_count;++i){BwProp *o=&g->props[i];if(!o->active)continue;if(o->position.y<o->half){o->position.y=o->half;o->previous_position.y=o->position.y;}for(int bi=0;bi<g->block_count;++bi){const BwBlock *v=&g->blocks[bi];if(v->spike||v->color==0)continue;bool over=o->position.x+o->half>v->position.x-v->half&&o->position.x-o->half<v->position.x+v->half&&o->position.z+o->half>v->position.z-v->half&&o->position.z-o->half<v->position.z+v->half;float top=v->position.y+v->half;if(over&&o->position.y-o->half<top&&o->position.y+o->half>v->position.y-v->half){if(o->position.y<o->previous_position.y+.001f){o->position.y=top+o->half;o->previous_position.y=o->position.y;}float fr=bw_clamp(v->ice?.03f:v->friction,0,1);o->previous_position.x=o->position.x-(o->position.x-o->previous_position.x)*fr;o->previous_position.z=o->position.z-(o->position.z-o->previous_position.z)*fr;}}}}
for(int i=0;i<g->prop_count;++i){BwProp *a=&g->props[i];if(!a->active)continue;for(int j=i+1;j<g->prop_count;++j){BwProp *b=&g->props[j];if(!b->active)continue;float dx=b->position.x-a->position.x,dz=b->position.z-a->position.z;if(fabsf(dx)<a->half+b->half&&fabsf(dz)<a->half+b->half&&fabsf(b->position.y-a->position.y)<a->half+b->half){float push=(a->half+b->half-fabsf(dx))*.5f*(dx<0?-1.0f:1.0f);a->position.x-=push;b->position.x+=push;}}}}

static void update_projectiles(BwGame *g,float dt){for(int q=0;q<64;++q){BwProjectile *s=&g->projectiles[q];if(!s->active)continue;s->life-=dt;if(s->life<=0){s->active=false;continue;}s->position.x+=s->velocity.x*dt;s->position.y+=s->velocity.y*dt;s->position.z+=s->velocity.z*dt;bool hit=false;for(int b=0;b<g->block_count&&!hit;++b){const BwBlock *v=&g->blocks[b];if(v->color&&s->position.x>v->position.x-v->half&&s->position.x<v->position.x+v->half&&s->position.y>v->position.y-v->half&&s->position.y<v->position.y+v->half&&s->position.z>v->position.z-v->half&&s->position.z<v->position.z+v->half)hit=true;}if(!s->npc_owned&&!hit)for(int n=0;n<g->npc_count&&!hit;++n){BwNpc *a=&g->npcs[n];if(a->active&&fabsf(s->position.x-a->position.x)<.5f&&fabsf(s->position.y-a->position.y-.8f)<1&&fabsf(s->position.z-a->position.z)<.5f){a->active=false;hit=true;}}if(hit)s->active=false;}}
void bw_players_update(BwGame *g,float dt){for(int i=0;i<BW_MAX_PLAYERS;++i)if(g->pads[i].joined&&g->pads[i].connected)update_one(g,i,dt);update_npcs(g,dt);update_props(g,dt);update_projectiles(g,dt);}
float bw_clamp(float v,float lo,float hi){return v<lo?lo:(v>hi?hi:v);}
void bw_quality_update(BwGame *g,float seconds){if(seconds<=0.0f)return;float fps=1.0f/seconds;g->fps_average=g->fps_average*0.90f+fps*0.10f;if(g->fps_average<48.0f&&g->quality_scale>0.55f)g->quality_scale-=0.05f;else if(g->fps_average>57.0f&&g->quality_scale<1.0f)g->quality_scale+=0.025f;g->quality_scale=bw_clamp(g->quality_scale,0.55f,1.0f);}

