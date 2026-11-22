#include "game/game.h"
BwGame g_game;
int main()
{
 ZeroMemory(&g_game,sizeof(g_game));
 g_game.editor_mode=true;g_game.show_grid=true;g_game.split_screen=false;g_game.controls_menu=false;g_game.running=true;
 g_game.quality_scale=1.0f;g_game.fps_average=60.0f;
 for(int i=0;i<BW_MAX_PLAYERS;++i){g_game.players[i].position=BwVec3((i-1.5f)*1.2f,BW_EYE_HEIGHT,-8.0f);g_game.players[i].grounded=true;g_game.active_slots[i]=i;}
 bw_world_init(&g_game);
 if(!bw_renderer_open())return 1;
 DWORD last=GetTickCount();
 while(g_game.running){
  DWORD now=GetTickCount(),elapsed=now-last;
  if(elapsed<16){Sleep(1);continue;}
  float dt=bw_clamp((float)elapsed/1000.0f,0.001f,0.05f);last=now;
  bw_quality_update(&g_game,dt);bw_input_poll(&g_game);
  for(int i=0;i<BW_MAX_PLAYERS;++i){
   if(g_game.pads[i].pressed_back)g_game.editor_mode=!g_game.editor_mode;
   if(g_game.pads[i].pressed_y)g_game.running=false;
   if(g_game.pads[i].pressed_start)g_game.controls_menu=!g_game.controls_menu;
   if(g_game.controls_menu&&g_game.pads[i].pressed_b)g_game.controls_menu=false;
  }
  if(g_game.running&&!g_game.controls_menu){
   bw_players_update(&g_game,dt);
   float split=g_game.player_count==1?1.0f:(g_game.player_count==2?0.72f:0.48f);
   g_game.render_distance=512.0f*split*g_game.quality_scale;
   bw_renderer_draw(&g_game);
  }else if(g_game.running){
   g_game.render_distance=512.0f*g_game.quality_scale;
   bw_renderer_draw(&g_game);
  }
 }
 bw_renderer_close();return 0;
}
