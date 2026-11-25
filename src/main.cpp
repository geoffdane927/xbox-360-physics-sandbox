#include "game/game.h"
BwGame g_game;
int main(){
 ZeroMemory(&g_game,sizeof(g_game));g_game.editor_mode=true;g_game.show_grid=true;g_game.running=true;g_game.render_distance=512;g_game.player_speed=5;g_game.jump_height=1.5f;g_game.gravity=18;g_game.physics_gravity=9.8f;g_game.physics_friction=0.985f;g_game.link_anchor_prop=-1;g_game.link_anchor_tool=-1;g_game.spawn_point=BwVec3(0,BW_EYE_HEIGHT,-8);g_game.quality_scale=1;g_game.fps_average=60;
 for(int i=0;i<BW_MAX_PLAYERS;++i){g_game.players[i].position=BwVec3((i-1.5f)*1.2f,BW_EYE_HEIGHT,-8);g_game.players[i].editor_mode=(i==0);g_game.players[i].menu_open=false;g_game.players[i].menu_selection=0;g_game.players[i].grounded=true;g_game.players[i].grabbed_prop=-1;g_game.players[i].grab_distance=5.0f;g_game.active_slots[i]=i;g_game.pads[i].joined=(i==0);}
 bw_world_init(&g_game);if(!bw_renderer_open())return 1;DWORD last=GetTickCount();
 while(g_game.running){DWORD now=GetTickCount(),elapsed=now-last;if(elapsed<16){Sleep(1);continue;}float dt=bw_clamp((float)elapsed/1000,.001f,.05f);last=now;bw_input_poll(&g_game);
  for(int i=0;i<BW_MAX_PLAYERS;++i){BwPad *pad=&g_game.pads[i];if(!pad->joined||!pad->connected)continue;BwPlayer *player=&g_game.players[i];
   if(player->menu_open){int *sel=&player->menu_selection;if(pad->pressed_b||pad->pressed_start)player->menu_open=false;if(pad->pressed_up)*sel=(*sel+14)%15;if(pad->pressed_down)*sel=(*sel+1)%15;int r=*sel;
    if(r==3){if(pad->pressed_left)g_game.render_distance=bw_clamp(g_game.render_distance-64,64,2048);if(pad->pressed_right)g_game.render_distance=bw_clamp(g_game.render_distance+64,64,2048);}
    else if(r==4){if(pad->pressed_left)g_game.player_speed=bw_clamp(g_game.player_speed-.5f,1,20);if(pad->pressed_right)g_game.player_speed=bw_clamp(g_game.player_speed+.5f,1,20);}
    else if(r==5){if(pad->pressed_left)g_game.jump_height=bw_clamp(g_game.jump_height-.25f,.5f,8);if(pad->pressed_right)g_game.jump_height=bw_clamp(g_game.jump_height+.25f,.5f,8);}
    else if(r==6){if(pad->pressed_left)g_game.gravity=bw_clamp(g_game.gravity-1,4,40);if(pad->pressed_right)g_game.gravity=bw_clamp(g_game.gravity+1,4,40);}
    else if(r==13){if(pad->pressed_left)g_game.physics_gravity=bw_clamp(g_game.physics_gravity-.5f,0,40);if(pad->pressed_right)g_game.physics_gravity=bw_clamp(g_game.physics_gravity+.5f,0,40);}
    else if(r==14){if(pad->pressed_left)g_game.physics_friction=bw_clamp(g_game.physics_friction-.02f,.50f,1);if(pad->pressed_right)g_game.physics_friction=bw_clamp(g_game.physics_friction+.02f,.50f,1);}
    else if(r>=7&&r<=9){float *v=r==7?&g_game.spawn_point.x:(r==8?&g_game.spawn_point.y:&g_game.spawn_point.z);if(pad->pressed_left)*v-=.5f;if(pad->pressed_right)*v+=.5f;}
    if(pad->pressed_a){if(r==0)player->menu_open=false;else if(r==1)player->editor_mode=!player->editor_mode;else if(r==2)g_game.automatic_render_distance=!g_game.automatic_render_distance;else if(r==10)g_game.spawn_point=player->position;else if(r==11)g_game.save_status=bw_world_save(&g_game)?1:-1;else if(r==12)g_game.save_status=bw_world_load(&g_game)?2:-2;}
   }else{if(pad->pressed_start)player->menu_open=true;else{if(pad->pressed_back)player->editor_mode=!player->editor_mode;if(player->editor_mode&&(pad->current.wButtons&XINPUT_GAMEPAD_LEFT_THUMB)&&!(pad->previous.wButtons&XINPUT_GAMEPAD_LEFT_THUMB))player->hotbar_page=(player->hotbar_page+1)%3;}}
  }
  g_game.player_count=0;for(int p=0;p<BW_MAX_PLAYERS;++p)if(g_game.pads[p].connected&&g_game.pads[p].joined)g_game.active_slots[g_game.player_count++]=p;if(g_game.player_count==0){g_game.player_count=1;g_game.active_slots[0]=0;}if(g_game.running)bw_players_update(&g_game,dt);if(g_game.automatic_render_distance)g_game.render_distance=(g_game.player_count<=1?512:(g_game.player_count==2?368:245))*g_game.quality_scale;if(g_game.running)bw_renderer_draw(&g_game);
 }
 bw_renderer_close();return 0;
}
