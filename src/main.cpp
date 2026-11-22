#include "game/game.h"
BwGame g_game;
static int object_count(const BwGame *g,int type){if(type==BW_OWNER_NPC)return g->npc_count;if(type==BW_OWNER_PROP)return g->prop_count;if(type==BW_OWNER_BLOCK)return g->block_count;if(type==BW_OWNER_PICKUP)return g->pickup_count;return 1;}
static int target_count(const BwGame *g,int type){if(type==0)return BW_MAX_PLAYERS;if(type==1)return g->npc_count;if(type==2)return g->prop_count;if(type==3)return g->block_count;return g->pickup_count;}
static void load_selected_rule(BwGame *g){int index=0;for(int q=0;q<g->rule_count;++q)if(g->rules[q].active&&g->rules[q].owner_type==g->script_owner_type&&g->rules[q].owner==g->script_owner){if(index++==g->script_rule_index){BwRule *r=&g->rules[q];g->script_condition=r->condition;g->script_action=r->action;g->script_target_type=r->target_type;g->script_target_index=r->target_index;g->script_summon_type=r->parameter;return;}}}
int main(){
 ZeroMemory(&g_game,sizeof(g_game));g_game.editor_mode=true;g_game.show_grid=true;g_game.running=true;g_game.render_distance=512;g_game.player_speed=5;g_game.jump_height=1.5f;g_game.gravity=18;g_game.spawn_point=BwVec3(0,BW_EYE_HEIGHT,-8);g_game.quality_scale=1;g_game.fps_average=60;g_game.script_owner_type=BW_OWNER_PLAYER;g_game.script_condition=BW_IF_SEE_PLAYER;g_game.script_action=BW_DO_CUSTOM_WEAPON;g_game.script_target_type=0;g_game.script_target_index=0;g_game.script_summon_type=0;g_game.script_rule_index=0;
 for(int i=0;i<BW_MAX_PLAYERS;++i){g_game.players[i].position=BwVec3((i-1.5f)*1.2f,BW_EYE_HEIGHT,-8);g_game.players[i].grounded=true;g_game.players[i].was_grounded=true;g_game.active_slots[i]=i;g_game.pads[i].joined=(i==0);}
 bw_world_init(&g_game);if(!bw_renderer_open())return 1;DWORD last=GetTickCount();
 while(g_game.running){
  DWORD now=GetTickCount(),elapsed=now-last;if(elapsed<16){Sleep(1);continue;}float dt=bw_clamp((float)elapsed/1000,0.001f,.05f);last=now;bw_input_poll(&g_game);
  for(int i=0;i<BW_MAX_PLAYERS;++i){BwPad *pad=&g_game.pads[i];if(!pad->joined)continue;
   if(pad->pressed_y){
    if(g_game.paused&&i==0){g_game.paused=false;g_game.script_editor=false;}
    else if(i==0&&g_game.editor_mode&&!g_game.paused){bw_editor_select_object(&g_game,i);}
    else if(!g_game.editor_mode)g_game.running=false;
   }
   if(i==0&&pad->pressed_start&&!g_game.paused){g_game.paused=true;g_game.script_editor=false;g_game.script_owner_type=BW_OWNER_PLAYER;g_game.script_owner=0;g_game.script_rule_index=0;load_selected_rule(&g_game);g_game.menu_selection=0;}
   if(g_game.paused&&i==0){
    if(pad->pressed_b){g_game.paused=false;g_game.script_editor=false;}
    if(pad->pressed_up)g_game.menu_selection=(g_game.menu_selection+22)%23;
    if(pad->pressed_down)g_game.menu_selection=(g_game.menu_selection+1)%23;
    int row=g_game.menu_selection;
    if(row==3){if(pad->pressed_left)g_game.render_distance=bw_clamp(g_game.render_distance-64,64,2048);if(pad->pressed_right)g_game.render_distance=bw_clamp(g_game.render_distance+64,64,2048);}
    else if(row==4){if(pad->pressed_left)g_game.player_speed=bw_clamp(g_game.player_speed-.5f,1,20);if(pad->pressed_right)g_game.player_speed=bw_clamp(g_game.player_speed+.5f,1,20);}
    else if(row==5){if(pad->pressed_left)g_game.jump_height=bw_clamp(g_game.jump_height-.25f,.5f,8);if(pad->pressed_right)g_game.jump_height=bw_clamp(g_game.jump_height+.25f,.5f,8);}
    else if(row==6){if(pad->pressed_left)g_game.gravity=bw_clamp(g_game.gravity-1,4,40);if(pad->pressed_right)g_game.gravity=bw_clamp(g_game.gravity+1,4,40);}
    else if(row>=7&&row<=9){float *v=row==7?&g_game.spawn_point.x:(row==8?&g_game.spawn_point.y:&g_game.spawn_point.z);if(pad->pressed_left)*v-=.5f;if(pad->pressed_right)*v+=.5f;*v=bw_clamp(*v,-999,999);}
    else if(row==13){if(pad->pressed_left)g_game.script_owner_type=(g_game.script_owner_type+4)%5;if(pad->pressed_right)g_game.script_owner_type=(g_game.script_owner_type+1)%5;int n=object_count(&g_game,g_game.script_owner_type);if(g_game.script_owner_type==BW_OWNER_PLAYER)g_game.script_owner=0;else if(n<=0)g_game.script_owner=0;else if(g_game.script_owner>=n)g_game.script_owner=n-1;g_game.script_rule_index=0;load_selected_rule(&g_game);}
    else if(row==14){if(pad->pressed_left)g_game.script_condition=(g_game.script_condition+11)%12;if(pad->pressed_right)g_game.script_condition=(g_game.script_condition+1)%12;}
    else if(row==15){if(pad->pressed_left)g_game.script_action=(g_game.script_action+11)%12;if(pad->pressed_right)g_game.script_action=(g_game.script_action+1)%12;}
    else if(row==16){if(pad->pressed_left)g_game.script_target_type=(g_game.script_target_type+4)%5;if(pad->pressed_right)g_game.script_target_type=(g_game.script_target_type+1)%5;int n=target_count(&g_game,g_game.script_target_type);if(n<=0)g_game.script_target_index=0;else if(g_game.script_target_index>=n)g_game.script_target_index=n-1;}
    else if(row==17){int n=object_count(&g_game,g_game.script_target_type);if(n<1)n=1;if(pad->pressed_left)g_game.script_target_index=(g_game.script_target_index+n-1)%n;if(pad->pressed_right)g_game.script_target_index=(g_game.script_target_index+1)%n;}
    else if(row==18){if(pad->pressed_left)g_game.script_summon_type=(g_game.script_summon_type+3)%4;if(pad->pressed_right)g_game.script_summon_type=(g_game.script_summon_type+1)%4;}
    else if(row==20){int count=0;for(int q=0;q<g_game.rule_count;++q)if(g_game.rules[q].active&&g_game.rules[q].owner_type==g_game.script_owner_type&&g_game.rules[q].owner==g_game.script_owner)++count;if(count>0){if(pad->pressed_left)g_game.script_rule_index=(g_game.script_rule_index+count-1)%count;if(pad->pressed_right)g_game.script_rule_index=(g_game.script_rule_index+1)%count;}else g_game.script_rule_index=0;load_selected_rule(&g_game);}
    if(pad->pressed_a){
     if(row==0){g_game.paused=false;g_game.script_editor=false;}
     else if(row==1)g_game.editor_mode=!g_game.editor_mode;
     else if(row==2)g_game.automatic_render_distance=!g_game.automatic_render_distance;
     else if(row==10)g_game.spawn_point=g_game.players[i].position;
     else if(row==11)g_game.save_status=bw_world_save(&g_game)?1:-1;
     else if(row==12)g_game.save_status=bw_world_load(&g_game)?2:-2;
     else if(row==19&&g_game.rule_count<BW_MAX_RULES){BwRule *rule=&g_game.rules[g_game.rule_count++];rule->owner_type=g_game.script_owner_type;rule->owner=g_game.script_owner;rule->condition=g_game.script_condition;rule->action=g_game.script_action;rule->target_type=g_game.script_target_type;rule->target_index=g_game.script_target_index;rule->parameter=g_game.script_summon_type;rule->active=true;rule->was_triggered=false;rule->timer=0.0f;}
     else if(row==21||row==22){int found=-1,index=0;for(int q=0;q<g_game.rule_count;++q)if(g_game.rules[q].active&&g_game.rules[q].owner_type==g_game.script_owner_type&&g_game.rules[q].owner==g_game.script_owner){if(index==g_game.script_rule_index){found=q;break;}++index;}if(found>=0){if(row==21){BwRule *rule=&g_game.rules[found];rule->condition=g_game.script_condition;rule->action=g_game.script_action;rule->target_type=g_game.script_target_type;rule->target_index=g_game.script_target_index;rule->parameter=g_game.script_summon_type;rule->was_triggered=false;rule->timer=0.0f;}else{for(int q=found;q<g_game.rule_count-1;++q)g_game.rules[q]=g_game.rules[q+1];--g_game.rule_count;if(g_game.script_rule_index>0)--g_game.script_rule_index;}}}
    }
   }else if(pad->pressed_back)g_game.editor_mode=!g_game.editor_mode;
  }
  g_game.player_count=0;for(int p=0;p<BW_MAX_PLAYERS;++p)if(g_game.pads[p].connected&&g_game.pads[p].joined)g_game.active_slots[g_game.player_count++]=p;
  if(g_game.player_count==0){g_game.player_count=1;g_game.active_slots[0]=0;}
  if(g_game.running&&!g_game.paused){bw_quality_update(&g_game,dt);bw_players_update(&g_game,dt);}
  if(g_game.automatic_render_distance){float split=g_game.player_count<=1?1:(g_game.player_count==2?.72f:.48f);g_game.render_distance=512*split*g_game.quality_scale;}
  if(g_game.running)bw_renderer_draw(&g_game);
 }
 bw_renderer_close();return 0;
}
