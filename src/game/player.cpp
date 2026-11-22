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
static bool npc_has_shot_rule(const BwGame *g,int n);
static void update_one(BwGame *g,int i,float dt)
{
 BwPlayer *p=&g->players[i];BwPad *pad=&g->pads[i];float speed=g->editor_mode?8.0f:g->player_speed;
 if(pad->pressed_left)p->selected_slot=(p->selected_slot+BW_INVENTORY_SLOTS-1)%BW_INVENTORY_SLOTS;
 if(pad->pressed_right)p->selected_slot=(p->selected_slot+1)%BW_INVENTORY_SLOTS;
 if(g->editor_mode&&pad->pressed_lthumb)p->hotbar_page=1-p->hotbar_page;
 if(g->editor_mode){if(pad->pressed_lb){if(p->hotbar_page==0)p->selected_slot=(p->selected_slot+7)%8;else p->selected_item=(p->selected_item+7)%8;}if(pad->pressed_rb){if(p->hotbar_page==0)p->selected_slot=(p->selected_slot+1)%8;else p->selected_item=(p->selected_item+1)%8;}}
 else{if(pad->pressed_lb)p->selected_weapon=(p->selected_weapon+4)%5;if(pad->pressed_rb)p->selected_weapon=(p->selected_weapon+1)%5;}
 if(p->weapon_anim>0.0f)p->weapon_anim-=dt;
 p->yaw+=pad->rx*1.8f*dt;p->pitch=bw_clamp(p->pitch+pad->ry*1.8f*dt,-1.45f,1.45f);
 float fx=sinf(p->yaw),fz=cosf(p->yaw),rx=cosf(p->yaw),rz=-sinf(p->yaw);
 float dx=(fx*pad->ly+rx*pad->lx)*speed*dt,dz=(fz*pad->ly+rz*pad->lx)*speed*dt;
 if(g->editor_mode){
  p->position.x+=dx;p->position.z+=dz;p->position.y+=(pad->rt-pad->lt)*speed*dt;
  if(pad->pressed_a){if(p->hotbar_page==0){BwVec3 q=editor_place_position(p);bw_world_add_block(g,q,0.5f,bw_slot_colors[p->selected_slot]);}
   else{BwVec3 q=editor_place_position(p);if(p->selected_item==0)bw_world_add_spike(g,q);else if(p->selected_item==1)bw_world_add_pickup(g,BwVec3(q.x,0.6f,q.z),1);else if(p->selected_item==2)bw_world_add_pickup(g,BwVec3(q.x,0.6f,q.z),2);else if(p->selected_item==3)g->spawn_point=BwVec3(q.x,BW_EYE_HEIGHT,q.z);else if(p->selected_item==4)bw_world_add_npc(g,BwVec3(q.x,0,q.z),0);else if(p->selected_item==5)bw_world_add_npc(g,BwVec3(q.x,0,q.z),1);else if(p->selected_item==6)bw_world_add_npc(g,BwVec3(q.x,0,q.z),2);else bw_world_add_prop(g,BwVec3(q.x,q.y,q.z));}}
  if(pad->pressed_x){if(g->block_count>0)--g->block_count;else if(g->prop_count>0)g->props[g->prop_count-1].active=false;}if(pad->pressed_b)g->show_grid=!g->show_grid;
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
  for(int k=0;k<g->prop_count;++k){BwProp *o=&g->props[k];if(!o->active)continue;float ox=o->position.x-p->position.x,oz=o->position.z-p->position.z,od=sqrtf(ox*ox+oz*oz);if(od<0.9f&&od>0.01f){o->velocity.x+=ox/od*3.5f*dt;o->velocity.z+=oz/od*3.5f*dt;}}
  for(int k=0;k<g->pickup_count;++k){BwPickup *it=&g->pickups[k];if(it->active&&fabsf(it->position.x-p->position.x)<0.8f&&fabsf(it->position.z-p->position.z)<0.8f){if(it->type==1){p->has_blaster=true;p->blaster_durability=100.0f;}if(it->type==2){p->has_sword=true;p->sword_durability=100.0f;}it->active=false;}}
  if(pad->pressed_x&&p->selected_weapon==2&&p->has_sword&&p->sword_durability>0.0f){p->weapon_anim=0.28f;p->sword_durability-=10.0f;if(p->sword_durability<=0)p->has_sword=false;float fx=sinf(p->yaw)*cosf(p->pitch),fz=cosf(p->yaw)*cosf(p->pitch);for(int t=0;t<g->player_count;++t){int target=g->active_slots[t];if(target==i)continue;BwPlayer *o=&g->players[target];float dx=o->position.x-p->position.x,dz=o->position.z-p->position.z,along=dx*fx+dz*fz,side=fabsf(dx*fz-dz*fx);if(along>0&&along<1.0f&&side<0.7f){o->position=g->spawn_point;o->vertical_velocity=0;o->grounded=true;o->died_event=true;}}for(int n=0;n<g->npc_count;++n){BwNpc *o=&g->npcs[n];float dx=o->position.x-p->position.x,dz=o->position.z-p->position.z,along=dx*fx+dz*fz,side=fabsf(dx*fz-dz*fx);if(o->active&&o->type>0&&along>0&&along<1.0f&&side<0.7f)o->active=false;}}
  if(pad->pressed_x&&p->selected_weapon==1&&p->has_blaster&&p->blaster_durability>0.0f){p->weapon_anim=0.18f;p->blaster_durability-=5.0f;if(p->blaster_durability<=0)p->has_blaster=false;for(int q=0;q<64;++q)if(!g->projectiles[q].active){BwProjectile *s=&g->projectiles[q];float cp=cosf(p->pitch);s->position=BwVec3(p->position.x+sinf(p->yaw)*cp*.65f,p->position.y+sinf(p->pitch)*.65f,p->position.z+cosf(p->yaw)*cp*.65f);s->velocity=BwVec3(sinf(p->yaw)*cp*28.0f,sinf(p->pitch)*28.0f,cosf(p->yaw)*cp*28.0f);s->owner=i;s->life=3.0f;s->active=true;s->npc_owned=false;break;}} }

}
static bool npc_has_shot_rule(const BwGame *g,int n){for(int r=0;r<g->rule_count;++r)if(g->rules[r].active&&g->rules[r].owner_type==BW_OWNER_NPC&&g->rules[r].owner==n&&(g->rules[r].condition==BW_IF_SHOT||g->rules[r].condition==BW_IF_ON_DEATH))return true;return false;}
static bool object_position(const BwGame *g,int type,int index,BwVec3 *out){
 if(type==0){if(index<0||index>=BW_MAX_PLAYERS)return false;*out=g->players[index].position;return true;}
 if(type==1){if(index<0||index>=g->npc_count||!g->npcs[index].active)return false;*out=g->npcs[index].position;return true;}
 if(type==2){if(index<0||index>=g->prop_count||!g->props[index].active)return false;*out=g->props[index].position;return true;}
 if(type==3){if(index<0||index>=g->block_count||g->blocks[index].color==0)return false;*out=g->blocks[index].position;return true;}
 if(type==4){if(index<0||index>=g->pickup_count||!g->pickups[index].active)return false;*out=g->pickups[index].position;return true;}
 return false;
}
static void set_object_position(BwGame *g,int type,int index,BwVec3 v){
 if(type==1&&index>=0&&index<g->npc_count)g->npcs[index].position=v;
 else if(type==2&&index>=0&&index<g->prop_count)g->props[index].position=v;
 else if(type==3&&index>=0&&index<g->block_count)g->blocks[index].position=v;
 else if(type==4&&index>=0&&index<g->pickup_count)g->pickups[index].position=v;
}
static bool object_alive(const BwGame *g,int type,int index){BwVec3 p;return object_position(g,type,index,&p);}
static void summon_from_rule(BwGame *g,const BwRule *r,BwVec3 at){
 int type=r->parameter;
 int newOwnerType=BW_OWNER_NPC,newIndex=-1;
 if(type==3){bw_world_add_prop(g,at);newOwnerType=BW_OWNER_PROP;newIndex=g->prop_count-1;}
 else{bw_world_add_npc(g,at,type==0?0:(type==1?1:2));newIndex=g->npc_count-1;}
 if(newIndex<0)return;
 BwRule copies[BW_MAX_RULES];int count=0;
 for(int i=0;i<g->rule_count&&count<BW_MAX_RULES;++i)if(&g->rules[i]!=r&&g->rules[i].active&&g->rules[i].action!=BW_DO_SUMMON&&g->rules[i].owner_type==r->owner_type&&g->rules[i].owner==r->owner)copies[count++]=g->rules[i];
 for(int i=0;i<count&&g->rule_count<BW_MAX_RULES;++i){BwRule *dst=&g->rules[g->rule_count++];*dst=copies[i];dst->owner_type=newOwnerType;dst->owner=newIndex;dst->was_triggered=false;}
}
static void run_player_rules(BwGame *g,int i,bool wasGrounded,float dt){
 BwPlayer *p=&g->players[i];BwPad *pad=&g->pads[i];bool jumped=pad->pressed_a&&!p->grounded&&p->vertical_velocity>0.0f;
 for(int r=0;r<g->rule_count;++r){BwRule *rule=&g->rules[r];if(!rule->active||rule->owner_type!=BW_OWNER_PLAYER||rule->owner!=i)continue;
  bool trigger=false;float nearest=9999.0f;int target=-1;
  for(int t=0;t<g->player_count;++t){int pi=g->active_slots[t];if(pi==i)continue;float x=g->players[pi].position.x-p->position.x,z=g->players[pi].position.z-p->position.z,d=sqrtf(x*x+z*z);if(d<nearest){nearest=d;target=pi;}}
  if(rule->condition==BW_IF_SEE_PLAYER)trigger=target>=0&&nearest<12.0f;
  else if(rule->condition==BW_IF_BUMP_GREEN){float x=p->position.x+sinf(p->yaw)*.6f,z=p->position.z+cosf(p->yaw)*.6f;for(int b=0;b<g->block_count;++b)if(g->blocks[b].color==bw_slot_colors[0]&&fabsf(x-g->blocks[b].position.x)<.85f&&fabsf(z-g->blocks[b].position.z)<.85f)trigger=true;}
  else if(rule->condition==BW_IF_TOUCH_PLAYER)trigger=target>=0&&nearest<0.9f;
  else if(rule->condition==BW_IF_TIMER){rule->timer-=dt;if(rule->timer<=0.0f){trigger=true;rule->timer=2.0f;}}
  else if(rule->condition==BW_IF_ON_DEATH)trigger=p->died_event;
  else if(rule->condition==BW_IF_ON_FALL)trigger=!p->grounded&&p->vertical_velocity<0.0f;
  else if(rule->condition==BW_IF_ON_STOP_FALLING||rule->condition==BW_IF_ON_LAND)trigger=p->grounded&&!wasGrounded;
  else if(rule->condition==BW_IF_ON_JUMP)trigger=jumped;
  else if(rule->condition==BW_IF_ON_USE)trigger=pad->pressed_x;
  else if(rule->condition==BW_IF_NEAR_ITEM){for(int k=0;k<g->pickup_count;++k)if(g->pickups[k].active){float x=g->pickups[k].position.x-p->position.x,z=g->pickups[k].position.z-p->position.z;if(x*x+z*z<9.0f)trigger=true;}}
  if(!trigger){rule->was_triggered=false;continue;}
  if(rule->action==BW_DO_SUMMON||rule->action==BW_DO_PLACE_BLOCK||rule->action==BW_DO_SHOOT||rule->action==BW_DO_CUSTOM_WEAPON){if(rule->was_triggered&&rule->condition!=BW_IF_TIMER)continue;rule->was_triggered=true;}else if(!trigger)rule->was_triggered=false;
  if(rule->action==BW_DO_MOVE_CAMERA){float cp=cosf(p->pitch);p->position.x+=sinf(p->yaw)*cp*3.0f*dt;p->position.y+=sinf(p->pitch)*3.0f*dt;p->position.z+=cosf(p->yaw)*cp*3.0f*dt;}
  else if(rule->action==BW_DO_MOVE_TOWARD){BwVec3 goal;if(object_position(g,rule->target_type,rule->target_index,&goal)){float x=goal.x-p->position.x,z=goal.z-p->position.z,d=sqrtf(x*x+z*z);if(d>0.1f){p->position.x+=x/d*3.0f*dt;p->position.z+=z/d*3.0f*dt;}}}
  else if(rule->action==BW_DO_MOVE_BACKWARD){p->position.x-=sinf(p->yaw)*2.0f*dt;p->position.z-=cosf(p->yaw)*2.0f*dt;}
  else if(rule->action==BW_DO_TURN)p->yaw+=3.0f*dt;
  else if(rule->action==BW_DO_JUMP&&p->grounded){p->vertical_velocity=sqrtf(2.0f*g->gravity*g->jump_height);p->grounded=false;}
  else if(rule->action==BW_DO_RESPAWN){p->position=g->spawn_point;p->vertical_velocity=0;p->grounded=true;}
  else if(rule->action==BW_DO_PLACE_BLOCK){BwVec3 at(p->position.x+sinf(p->yaw)*2.0f,0.5f,p->position.z+cosf(p->yaw)*2.0f);bw_world_add_block(g,at,.5f,bw_slot_colors[p->selected_slot]);}
  else if(rule->action==BW_DO_SUMMON){BwVec3 at(p->position.x+sinf(p->yaw)*3.0f,0.6f,p->position.z+cosf(p->yaw)*3.0f);summon_from_rule(g,rule,at);}
  else if(rule->action==BW_DO_SHOOT||rule->action==BW_DO_CUSTOM_WEAPON){for(int q=0;q<64;++q)if(!g->projectiles[q].active){BwProjectile *s=&g->projectiles[q];float cp=cosf(p->pitch);s->position=BwVec3(p->position.x+sinf(p->yaw)*cp*.6f,p->position.y+sinf(p->pitch)*.6f,p->position.z+cosf(p->yaw)*cp*.6f);s->velocity=BwVec3(sinf(p->yaw)*cp*30.0f,sinf(p->pitch)*30.0f,cosf(p->yaw)*cp*30.0f);s->owner=i;s->life=3.0f;s->active=true;s->npc_owned=false;p->weapon_anim=.18f;break;}}
 }
 p->died_event=false;
}
static bool npc_blocked(const BwGame *g,float x,float z){for(int b=0;b<g->block_count;++b){const BwBlock *v=&g->blocks[b];if(v->color!=0&&!v->spike&&x>v->position.x-v->half-.32f&&x<v->position.x+v->half+.32f&&z>v->position.z-v->half-.32f&&z<v->position.z+v->half+.32f)return true;}return false;}
static bool npc_bumps_green(const BwGame *g,const BwNpc *a,float dx,float dz){float x=a->position.x+dx,z=a->position.z+dz;for(int b=0;b<g->block_count;++b){const BwBlock *v=&g->blocks[b];if(v->color!=0&&!v->spike&&v->color==bw_slot_colors[0]&&x>v->position.x-v->half-.35f&&x<v->position.x+v->half+.35f&&z>v->position.z-v->half-.35f&&z<v->position.z+v->half+.35f)return true;}return false;}
static void update_npcs(BwGame *g,float dt){
 for(int n=0;n<g->npc_count;++n){BwNpc *a=&g->npcs[n];if(!a->active)continue;a->anim+=dt;a->cooldown-=dt;
  int target=g->player_count>0?g->active_slots[0]:0;BwPlayer *p=&g->players[target];
  float dx=p->position.x-a->position.x,dz=p->position.z-a->position.z,d=sqrtf(dx*dx+dz*dz);if(d>0.001f)a->yaw=(float)atan2(dx,dz);
  bool scripted=false;
  for(int r=0;r<g->rule_count;++r){BwRule *rule=&g->rules[r];if(!rule->active||rule->owner_type!=BW_OWNER_NPC||rule->owner!=n)continue;bool trigger=false;
   if(rule->condition==BW_IF_SEE_PLAYER)trigger=g->player_count>0&&d<12.0f;
   else if(rule->condition==BW_IF_BUMP_GREEN)trigger=npc_bumps_green(g,a,sinf(a->yaw)*1.5f*dt,cosf(a->yaw)*1.5f*dt);
   else if(rule->condition==BW_IF_SHOT||rule->condition==BW_IF_ON_DEATH)trigger=a->was_shot;
   else if(rule->condition==BW_IF_TOUCH_PLAYER)trigger=g->player_count>0&&d<0.9f;
   else if(rule->condition==BW_IF_TIMER){rule->timer-=dt;if(rule->timer<=0.0f){trigger=true;rule->timer=2.0f;}}
   else if(rule->condition==BW_IF_ON_FALL)trigger=a->vertical_velocity<0.0f&&a->position.y>0.0f;
   else if(rule->condition==BW_IF_ON_STOP_FALLING||rule->condition==BW_IF_ON_LAND)trigger=a->was_falling&&a->vertical_velocity==0.0f;
   else if(rule->condition==BW_IF_NEAR_ITEM){for(int k=0;k<g->pickup_count;++k)if(g->pickups[k].active){float ix=g->pickups[k].position.x-a->position.x,iz=g->pickups[k].position.z-a->position.z;if(ix*ix+iz*iz<9.0f)trigger=true;}}
   if(!trigger){rule->was_triggered=false;continue;}scripted=true;
   if(rule->action==BW_DO_SUMMON||rule->action==BW_DO_PLACE_BLOCK||rule->action==BW_DO_SHOOT||rule->action==BW_DO_CUSTOM_WEAPON){if(rule->was_triggered&&rule->condition!=BW_IF_TIMER)continue;rule->was_triggered=true;}
   BwVec3 goal;bool hasGoal=object_position(g,rule->target_type,rule->target_index,&goal);
   if(rule->action==BW_DO_MOVE_TOWARD&&hasGoal){float tx=goal.x-a->position.x,tz=goal.z-a->position.z,td=sqrtf(tx*tx+tz*tz);if(td>.01f){float nx=a->position.x+tx/td*2.0f*dt,nz=a->position.z+tz/td*2.0f*dt;if(!npc_blocked(g,nx,nz)){a->position.x=nx;a->position.z=nz;}}}
   else if(rule->action==BW_DO_MOVE_BACKWARD){float nx=a->position.x-sinf(a->yaw)*2.0f*dt,nz=a->position.z-cosf(a->yaw)*2.0f*dt;if(!npc_blocked(g,nx,nz)){a->position.x=nx;a->position.z=nz;}}
   else if(rule->action==BW_DO_MOVE_CAMERA){a->position.x+=sinf(a->yaw)*2.0f*dt;a->position.z+=cosf(a->yaw)*2.0f*dt;}
   else if(rule->action==BW_DO_TURN)a->yaw+=3.0f*dt;
   else if(rule->action==BW_DO_GREEN)a->color=bw_slot_colors[0];
   else if(rule->action==BW_DO_DISAPPEAR)a->active=false;
   else if(rule->action==BW_DO_RESPAWN){a->position=a->spawn_position;a->vertical_velocity=0.0f;}
   else if(rule->action==BW_DO_JUMP)a->vertical_velocity=5.0f;
   else if(rule->action==BW_DO_PLACE_BLOCK){BwVec3 at(a->position.x+sinf(a->yaw)*2.0f,.5f,a->position.z+cosf(a->yaw)*2.0f);bw_world_add_block(g,at,.5f,bw_slot_colors[0]);}
   else if(rule->action==BW_DO_SUMMON)summon_from_rule(g,rule,BwVec3(a->position.x+sinf(a->yaw)*2.5f,.6f,a->position.z+cosf(a->yaw)*2.5f));
   else if((rule->action==BW_DO_SHOOT||rule->action==BW_DO_CUSTOM_WEAPON)&&g->player_count>0&&(a->cooldown<=0.0f||rule->condition==BW_IF_TIMER)){for(int q=0;q<64;++q)if(!g->projectiles[q].active){BwProjectile *shot=&g->projectiles[q];shot->position=BwVec3(a->position.x,1.2f,a->position.z);float tx=hasGoal?goal.x-p->position.x:dx,tz=hasGoal?goal.z-p->position.z:dz;float td=sqrtf(tx*tx+tz*tz);shot->velocity=BwVec3(tx/(td>.01f?td:1)*18.0f,0,tz/(td>.01f?td:1)*18.0f);shot->owner=-1;shot->life=2.5f;shot->active=true;shot->npc_owned=true;a->cooldown=1.0f;break;}}
  }
  a->was_shot=false;
  if(a->vertical_velocity!=0.0f||a->position.y>0.0f){a->vertical_velocity-=9.8f*dt;a->position.y+=a->vertical_velocity*dt;if(a->position.y<0.0f){a->position.y=0.0f;a->vertical_velocity=0.0f;}}
  a->was_falling=a->vertical_velocity<0.0f;
  if(scripted)continue;if(g->player_count<=0)continue;
  if(a->type==0){if(d>2.5f&&d<20.0f){float nx=a->position.x+dx/d*1.2f*dt,nz=a->position.z+dz/d*1.2f*dt;if(!npc_blocked(g,nx,nz)){a->position.x=nx;a->position.z=nz;}}continue;}
  if(a->type==1){if(d>0.85f){float nx=a->position.x+dx/d*1.8f*dt,nz=a->position.z+dz/d*1.8f*dt;if(!npc_blocked(g,nx,nz)){a->position.x=nx;a->position.z=nz;}}else if(a->cooldown<=0){p->position=g->spawn_point;p->vertical_velocity=0;p->grounded=true;p->died_event=true;a->cooldown=1.0f;}}
  else if(a->type==2&&a->cooldown<=0&&d<18.0f){for(int q=0;q<64;++q)if(!g->projectiles[q].active){BwProjectile *shot=&g->projectiles[q];shot->position=BwVec3(a->position.x,1.25f,a->position.z);shot->velocity=BwVec3(dx/d*18.0f,0,dz/d*18.0f);shot->owner=-1;shot->life=2.5f;shot->active=true;shot->npc_owned=true;a->cooldown=1.5f;break;}}
 }
}
static void update_props(BwGame *g,float dt){
 for(int i=0;i<g->prop_count;++i){BwProp *o=&g->props[i];if(!o->active)continue;
  o->velocity.y-=9.8f*dt;o->position.x+=o->velocity.x*dt;o->position.z+=o->velocity.z*dt;
  for(int b=0;b<g->block_count;++b){const BwBlock *v=&g->blocks[b];if(v->spike||v->color==0)continue;if(o->position.x+o->half>v->position.x-v->half&&o->position.x-o->half<v->position.x+v->half&&o->position.z+o->half>v->position.z-v->half&&o->position.z-o->half<v->position.z+v->half&&o->position.y-o->half<v->position.y+v->half&&o->position.y+o->half>v->position.y-v->half){o->position.x-=o->velocity.x*dt;o->position.z-=o->velocity.z*dt;o->velocity.x=0;o->velocity.z=0;}}
  float oldY=o->position.y;o->position.y+=o->velocity.y*dt;if(o->velocity.y<=0.0f){float landing=o->half;for(int b=0;b<g->block_count;++b){const BwBlock *v=&g->blocks[b];if(v->spike||v->color==0)continue;bool over=o->position.x+o->half>v->position.x-v->half&&o->position.x-o->half<v->position.x+v->half&&o->position.z+o->half>v->position.z-v->half&&o->position.z-o->half<v->position.z+v->half;float top=v->position.y+v->half;if(over&&oldY-o->half>=top-0.02f&&o->position.y-o->half<=top&&top+o->half>landing)landing=top+o->half;}if(o->position.y<landing){o->position.y=landing;o->velocity.y=0;}}
  bool falling=o->velocity.y<-.05f,stopped=o->was_falling&&!falling;o->was_falling=falling;o->velocity.x*=0.985f;o->velocity.z*=0.985f;
  for(int r=0;r<g->rule_count;++r){BwRule *rule=&g->rules[r];if(!rule->active||rule->owner_type!=BW_OWNER_PROP||rule->owner!=i)continue;bool trigger=false;float nearest=9999.0f;BwVec3 goal;
   for(int t=0;t<g->player_count;++t){BwPlayer *pl=&g->players[g->active_slots[t]];float x=pl->position.x-o->position.x,z=pl->position.z-o->position.z,d=sqrtf(x*x+z*z);if(d<nearest){nearest=d;goal=pl->position;}}
   if(rule->condition==BW_IF_SEE_PLAYER)trigger=nearest<12.0f;else if(rule->condition==BW_IF_TOUCH_PLAYER)trigger=nearest<.9f;else if(rule->condition==BW_IF_BUMP_GREEN){for(int b=0;b<g->block_count;++b)if(g->blocks[b].color==bw_slot_colors[0]&&fabsf(g->blocks[b].position.x-o->position.x)<.9f&&fabsf(g->blocks[b].position.z-o->position.z)<.9f)trigger=true;}else if(rule->condition==BW_IF_ON_FALL)trigger=falling;else if(rule->condition==BW_IF_ON_STOP_FALLING||rule->condition==BW_IF_ON_LAND)trigger=stopped;else if(rule->condition==BW_IF_TIMER){rule->timer-=dt;if(rule->timer<=0){trigger=true;rule->timer=2.0f;}}
   else if(rule->condition==BW_IF_NEAR_ITEM){for(int k=0;k<g->pickup_count;++k)if(g->pickups[k].active){float x=g->pickups[k].position.x-o->position.x,z=g->pickups[k].position.z-o->position.z;if(x*x+z*z<9)trigger=true;}}
   if(!trigger){rule->was_triggered=false;continue;}
   BwVec3 target;bool has=object_position(g,rule->target_type,rule->target_index,&target);
   if(rule->action==BW_DO_MOVE_TOWARD&&has){float x=target.x-o->position.x,z=target.z-o->position.z,d=sqrtf(x*x+z*z);if(d>.01f){o->velocity.x=x/d*2.0f;o->velocity.z=z/d*2.0f;}}
   else if(rule->action==BW_DO_MOVE_CAMERA){o->velocity.x+=goal.x>o->position.x?1.0f:-1.0f;o->velocity.z+=goal.z>o->position.z?1.0f:-1.0f;}
   else if(rule->action==BW_DO_GREEN)o->color=bw_slot_colors[0];
   else if(rule->action==BW_DO_DISAPPEAR)o->active=false;
   else if(rule->action==BW_DO_RESPAWN){o->position=o->spawn_position;o->velocity=BwVec3(0,0,0);}
   else if(rule->action==BW_DO_SUMMON)summon_from_rule(g,rule,BwVec3(o->position.x+1.5f,o->position.y+.6f,o->position.z));
   else if(rule->action==BW_DO_PLACE_BLOCK)bw_world_add_block(g,BwVec3(o->position.x,0.5f,o->position.z),.5f,bw_slot_colors[0]);
  }
 }
}
static void update_block_and_pickup_rules(BwGame *g,float dt){
 for(int r=0;r<g->rule_count;++r){BwRule *rule=&g->rules[r];if(!rule->active||(rule->owner_type!=BW_OWNER_BLOCK&&rule->owner_type!=BW_OWNER_PICKUP))continue;BwVec3 at;if(!object_position(g,rule->owner_type==BW_OWNER_BLOCK?3:4,rule->owner,&at))continue;bool trigger=false;float nearest=9999.0f;int player=-1;
  for(int t=0;t<g->player_count;++t){int pi=g->active_slots[t];float x=g->players[pi].position.x-at.x,z=g->players[pi].position.z-at.z,d=sqrtf(x*x+z*z);if(d<nearest){nearest=d;player=pi;}}
  if(rule->condition==BW_IF_SEE_PLAYER)trigger=nearest<12.0f;else if(rule->condition==BW_IF_TOUCH_PLAYER)trigger=nearest<1.0f;else if(rule->condition==BW_IF_BUMP_GREEN&&rule->owner_type==BW_OWNER_BLOCK)trigger=g->blocks[rule->owner].color==bw_slot_colors[0];else if(rule->condition==BW_IF_TIMER){rule->timer-=dt;if(rule->timer<=0){trigger=true;rule->timer=2.0f;}}
  else if(rule->condition==BW_IF_NEAR_ITEM){for(int k=0;k<g->pickup_count;++k)if(g->pickups[k].active){float x=g->pickups[k].position.x-at.x,z=g->pickups[k].position.z-at.z;if(x*x+z*z<9.0f)trigger=true;}}
  if(!trigger){rule->was_triggered=false;continue;}if(rule->action==BW_DO_SUMMON||rule->action==BW_DO_PLACE_BLOCK||rule->action==BW_DO_SHOOT||rule->action==BW_DO_CUSTOM_WEAPON){if(rule->was_triggered&&rule->condition!=BW_IF_TIMER)continue;rule->was_triggered=true;}BwVec3 target;bool has=object_position(g,rule->target_type,rule->target_index,&target);
  if(rule->action==BW_DO_MOVE_TOWARD&&has){float x=target.x-at.x,z=target.z-at.z,d=sqrtf(x*x+z*z);if(d>.01f){at.x+=x/d*2.0f*dt;at.z+=z/d*2.0f*dt;set_object_position(g,rule->owner_type==BW_OWNER_BLOCK?3:4,rule->owner,at);}}
  else if(rule->action==BW_DO_GREEN&&rule->owner_type==BW_OWNER_BLOCK)g->blocks[rule->owner].color=bw_slot_colors[0];
  else if(rule->action==BW_DO_DISAPPEAR&&rule->owner_type==BW_OWNER_BLOCK)g->blocks[rule->owner].color=0;
  else if(rule->action==BW_DO_DISAPPEAR&&rule->owner_type==BW_OWNER_PICKUP)g->pickups[rule->owner].active=false;
  else if(rule->action==BW_DO_SUMMON)summon_from_rule(g,rule,BwVec3(at.x+1.5f,at.y+.6f,at.z));
  else if(rule->action==BW_DO_PLACE_BLOCK)bw_world_add_block(g,BwVec3(at.x,0.5f,at.z),.5f,bw_slot_colors[0]);
  else if(rule->action==BW_DO_MOVE_CAMERA&&player>=0){BwPlayer *pl=&g->players[player];at.x+=sinf(pl->yaw)*2.0f*dt;at.z+=cosf(pl->yaw)*2.0f*dt;set_object_position(g,rule->owner_type==BW_OWNER_BLOCK?3:4,rule->owner,at);}
 }
}

void bw_players_update(BwGame *g,float dt){for(int i=0;i<BW_MAX_PLAYERS;++i)if(g->pads[i].joined&&g->pads[i].connected){bool wasGrounded=g->players[i].grounded;update_one(g,i,dt);run_player_rules(g,i,wasGrounded,dt);}update_npcs(g,dt);update_props(g,dt);update_block_and_pickup_rules(g,dt);update_projectiles(g,dt);}
float bw_clamp(float v,float lo,float hi){return v<lo?lo:(v>hi?hi:v);}
void bw_quality_update(BwGame *g,float seconds){if(seconds<=0.0f)return;float fps=1.0f/seconds;g->fps_average=g->fps_average*0.90f+fps*0.10f;if(g->fps_average<48.0f&&g->quality_scale>0.55f)g->quality_scale-=0.05f;else if(g->fps_average>57.0f&&g->quality_scale<1.0f)g->quality_scale+=0.025f;g->quality_scale=bw_clamp(g->quality_scale,0.55f,1.0f);}

bool bw_editor_select_object(BwGame *g,int player){
 if(player<0||player>=BW_MAX_PLAYERS)return false;
 BwPlayer *p=&g->players[player];float cp=cosf(p->pitch),dx=sinf(p->yaw)*cp,dy=sinf(p->pitch),dz=cosf(p->yaw)*cp;
 float best=9999.0f;int bestType=-1,bestIndex=-1;
 for(int type=0;type<4;++type){int count=type==0?g->npc_count:(type==1?g->prop_count:(type==2?g->block_count:g->pickup_count));
  for(int i=0;i<count;++i){BwVec3 q;bool active=true;
   if(type==0){active=g->npcs[i].active;q=g->npcs[i].position;q.y+=.9f;}
   else if(type==1){active=g->props[i].active;q=g->props[i].position;}
   else if(type==2){active=g->blocks[i].color!=0;q=g->blocks[i].position;}
   else{active=g->pickups[i].active;q=g->pickups[i].position;}
   if(!active)continue;float vx=q.x-p->position.x,vy=q.y-p->position.y,vz=q.z-p->position.z;
   float along=vx*dx+vy*dy+vz*dz;if(along<0.0f||along>14.0f)continue;
   float ex=vx-dx*along,ey=vy-dy*along,ez=vz-dz*along;float miss=sqrtf(ex*ex+ey*ey+ez*ez);
   float radius=type==0?0.8f:(type==1?0.85f:(type==2?0.9f:0.65f));
   if(miss<radius&&along<best){best=along;bestType=type;bestIndex=i;}
  }
 }
 if(bestType<0)return false;
 g->selected_object_type=bestType;g->selected_object_index=bestIndex;
 g->script_owner_type=bestType==0?BW_OWNER_NPC:(bestType==1?BW_OWNER_PROP:(bestType==2?BW_OWNER_BLOCK:BW_OWNER_PICKUP));
 g->script_owner=bestIndex;g->script_rule_index=0;int found=0;for(int r=0;r<g->rule_count;++r)if(g->rules[r].active&&g->rules[r].owner_type==g->script_owner_type&&g->rules[r].owner==g->script_owner){if(found++==0){g->script_condition=g->rules[r].condition;g->script_action=g->rules[r].action;g->script_target_type=g->rules[r].target_type;g->script_target_index=g->rules[r].target_index;g->script_summon_type=g->rules[r].parameter;break;}}g->script_editor=true;g->paused=true;g->menu_selection=13;
 return true;
}
