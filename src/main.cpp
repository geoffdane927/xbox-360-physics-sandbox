#include "game/game.h"
BwGame g_game;
int main()
{
 ZeroMemory(&g_game,sizeof(g_game));
 g_game.editor_mode=true;g_game.show_grid=true;g_game.split_screen=false;
 g_game.paused=false;g_game.automatic_render_distance=false;g_game.running=true;
 g_game.render_distance=512.0f;g_game.menu_selection=0;
 g_game.quality_scale=1.0f;g_game.fps_average=60.0f;
 for(int i=0;i<BW_MAX_PLAYERS;++i){g_game.players[i].position=BwVec3((i-1.5f)*1.2f,BW_EYE_HEIGHT,-8.0f);g_game.players[i].grounded=true;g_game.active_slots[i]=i;}
 bw_world_init(&g_game);
 if(!bw_renderer_open())return 1;
 DWORD last=GetTickCount();
 while(g_game.running){
  DWORD now=GetTickCount(),elapsed=now-last;
  if(elapsed<16){Sleep(1);continue;}
  float dt=bw_clamp((float)elapsed/1000.0f,0.001f,0.05f);last=now;
  bw_input_poll(&g_game);
  for(int i=0;i<BW_MAX_PLAYERS;++i){
   BwPad *pad=&g_game.pads[i];
   if(pad->pressed_y)g_game.running=false;
   if(pad->pressed_start)g_game.paused=!g_game.paused;
   if(g_game.paused){
    if(pad->pressed_b)g_game.paused=false;
    if(pad->pressed_up)g_game.menu_selection=(g_game.menu_selection+3)%4;
    if(pad->pressed_down)g_game.menu_selection=(g_game.menu_selection+1)%4;
    if(g_game.menu_selection==3){
     if(pad->pressed_left)g_game.render_distance=bw_clamp(g_game.render_distance-64.0f,64.0f,2048.0f);
     if(pad->pressed_right)g_game.render_distance=bw_clamp(g_game.render_distance+64.0f,64.0f,2048.0f);
    }
    if(pad->pressed_a){
     if(g_game.menu_selection==0)g_game.paused=false;
     else if(g_game.menu_selection==1)g_game.editor_mode=!g_game.editor_mode;
     else if(g_game.menu_selection==2)g_game.automatic_render_distance=!g_game.automatic_render_distance;
    }
   }else if(pad->pressed_back)g_game.editor_mode=!g_game.editor_mode;
  }
  if(g_game.running&&!g_game.paused){
   bw_quality_update(&g_game,dt);
   bw_players_update(&g_game,dt);
  }
  if(g_game.automatic_render_distance){
   float split=g_game.player_count<=1?1.0f:(g_game.player_count==2?0.72f:0.48f);
   g_game.render_distance=512.0f*split*g_game.quality_scale;
  }
  if(g_game.running)bw_renderer_draw(&g_game);
 }
 bw_renderer_close();return 0;
}
