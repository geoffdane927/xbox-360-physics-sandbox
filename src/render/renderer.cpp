#include "game/game.h"
#include <d3dx9.h>
#include <math.h>
#include <string.h>

static const int SW=1280,SH=720,MAX_TRI=24576,MAX_LINE=8192;
static const float PI=3.14159265358979323846f;
struct Vtx { float x,y,z,w; DWORD color; };
struct View { int x,y,w,h; };
static Vtx tris[MAX_TRI],lines[MAX_LINE];
static int nt=0,nl=0;
static IDirect3D9 *d3d=0;
static IDirect3DDevice9 *dev=0;
static IDirect3DVertexShader9 *vertex_shader=0;
static IDirect3DPixelShader9 *pixel_shader=0;
static IDirect3DVertexDeclaration9 *vertex_decl=0;

static const unsigned char font5x7[39][5]={
 {0x7e,0x11,0x11,0x11,0x7e}, {0x7f,0x49,0x49,0x49,0x36},
 {0x3e,0x41,0x41,0x41,0x22}, {0x7f,0x41,0x41,0x22,0x1c},
 {0x7f,0x49,0x49,0x49,0x41}, {0x7f,0x09,0x09,0x09,0x01},
 {0x3e,0x41,0x49,0x49,0x7a}, {0x7f,0x08,0x08,0x08,0x7f},
 {0x00,0x41,0x7f,0x41,0x00}, {0x20,0x40,0x41,0x3f,0x01},
 {0x7f,0x08,0x14,0x22,0x41}, {0x7f,0x40,0x40,0x40,0x40},
 {0x7f,0x02,0x0c,0x02,0x7f}, {0x7f,0x04,0x08,0x10,0x7f},
 {0x3e,0x41,0x41,0x41,0x3e}, {0x7f,0x09,0x09,0x09,0x06},
 {0x3e,0x41,0x51,0x21,0x5e}, {0x7f,0x09,0x19,0x29,0x46},
 {0x26,0x49,0x49,0x49,0x32}, {0x01,0x01,0x7f,0x01,0x01},
 {0x3f,0x40,0x40,0x40,0x3f}, {0x1f,0x20,0x40,0x20,0x1f},
 {0x3f,0x40,0x38,0x40,0x3f}, {0x63,0x14,0x08,0x14,0x63},
 {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43},
 {0x3e,0x51,0x49,0x45,0x3e}, {0x00,0x42,0x7f,0x40,0x00},
 {0x62,0x51,0x49,0x49,0x46}, {0x22,0x41,0x49,0x49,0x36},
 {0x18,0x14,0x12,0x7f,0x10}, {0x2f,0x49,0x49,0x49,0x31},
 {0x3e,0x49,0x49,0x49,0x32}, {0x01,0x71,0x09,0x05,0x03},
 {0x36,0x49,0x49,0x49,0x36}, {0x26,0x49,0x49,0x49,0x3e},
 {0x00,0x36,0x36,0x00,0x00}, {0x00,0x60,0x60,0x00,0x00}, {0x08,0x08,0x08,0x08,0x08}
};
static int glyph_index(char c){
 if(c>='A'&&c<='Z')return c-'A';
 if(c>='0'&&c<='9')return 26+c-'0';
 if(c==':')return 36;
 if(c=='.')return 37;
 if(c=='-')return 38;
 return -1;
}
static Vtx menu_v[90000]; static int menu_n=0;
static void menu_pixel(float x,float y,float scale,DWORD color){
 if(menu_n+6>90000)return;
 float l=(x/(float)SW)*2.0f-1.0f,r=((x+scale)/(float)SW)*2.0f-1.0f;
 float t=1.0f-(y/(float)SH)*2.0f,b=1.0f-((y+scale)/(float)SH)*2.0f;
 Vtx *v=&menu_v[menu_n];
 const float px[6]={l,r,r,l,r,l},py[6]={t,t,b,t,b,b};
 for(int i=0;i<6;++i){v[i].x=px[i];v[i].y=py[i];v[i].z=0;v[i].w=1;v[i].color=color;}
 menu_n+=6;
}
static void menu_rect(float x,float y,float w,float h,DWORD color){
 if(menu_n+6>90000)return;
 float l=(x/(float)SW)*2.0f-1.0f,r=((x+w)/(float)SW)*2.0f-1.0f;
 float t=1.0f-(y/(float)SH)*2.0f,b=1.0f-((y+h)/(float)SH)*2.0f;
 const float px[6]={l,r,r,l,r,l},py[6]={t,t,b,t,b,b};
 for(int i=0;i<6;++i){menu_v[menu_n+i].x=px[i];menu_v[menu_n+i].y=py[i];menu_v[menu_n+i].z=0;menu_v[menu_n+i].w=1;menu_v[menu_n+i].color=color;}
 menu_n+=6;
}
static void menu_text(float x,float y,const char *s,float scale,DWORD color){
 for(;*s;++s,x+=6*scale){int g=glyph_index(*s);if(g<0)continue;
  for(int col=0;col<5;++col)for(int row=0;row<7;++row)
   if(font5x7[g][col]&(1<<row))menu_pixel(x+col*scale,y+row*scale,scale,color);
 }
}
static void format_tenths(float value,char *out){
 int n=(int)(value*10.0f+0.5f);
 out[0]=(char)('0'+(n/100)%10);
 out[1]=(char)('0'+(n/10)%10);
 out[2]='.';
 out[3]=(char)('0'+n%10);
 out[4]=0;
}
static void format_signed_tenths(float value,char *out){
 int n=(int)(fabsf(value)*10.0f+0.5f),pos=0;
 if(value<0.0f)out[pos++]='-';
 if(n>=1000)out[pos++]=(char)('0'+(n/1000)%10);
 out[pos++]=(char)('0'+(n/100)%10);out[pos++]=(char)('0'+(n/10)%10);
 out[pos++]='.';out[pos++]=(char)('0'+n%10);out[pos]=0;
}
static void controls_menu(const BwGame *g){
 menu_n=0;DWORD white=D3DCOLOR_ARGB(255,235,242,250),muted=D3DCOLOR_ARGB(255,150,175,200);
 DWORD panel=D3DCOLOR_ARGB(255,12,20,32),accent=D3DCOLOR_ARGB(255,55,135,205),selected=D3DCOLOR_ARGB(255,28,65,98);
 menu_rect(145,35,990,650,panel);menu_rect(170,70,940,5,accent);
 menu_text(190,88,g->script_editor?"OBJECT SCRIPT EDITOR":"PAUSED / PLAYER SCRIPTING",4,white);
 const char *labels[23]={"RESUME","MODE","AUTO DISTANCE","RENDER DISTANCE","MOVE SPEED","JUMP HEIGHT","GRAVITY","SPAWN X","SPAWN Y","SPAWN Z","SET SPAWN HERE","SAVE GAME","LOAD GAME","SCRIPT OWNER","IF CONDITION","DO ACTION","MOVE TOWARD TYPE","MOVE TOWARD ID","SUMMON TYPE","ADD RULE","RULE TO EDIT","UPDATE RULE","DELETE RULE"};
 static const char *owners[5]={"NPC","CRATE","BLOCK","PICKUP","PLAYER"};
 static const char *conditions[12]={"SEE PLAYER","BUMP GREEN","GET SHOT","TOUCH PLAYER","TIMER","NEAR ITEM","ON DEATH","ON FALL","STOP FALLING","ON LAND","ON JUMP","ON USE"};
 static const char *actions[12]={"MOVE TOWARD","MOVE BACK","TURN AROUND","CHANGE GREEN","DISAPPEAR","RESPAWN","JUMP","SHOOT","MOVE CAMERA","SUMMON","PLACE BLOCK","CUSTOM WEAPON"};
 static const char *targets[5]={"PLAYER","NPC","CRATE","BLOCK","PICKUP"};
 static const char *summons[4]={"FRIENDLY NPC","SWORD NPC","BLASTER NPC","CRATE"};
 for(int i=0;i<23;++i){float y=122.0f+i*22.0f;if(g->menu_selection==i)menu_rect(180,y-3,560,23,selected);menu_text(195,y,labels[i],2,g->menu_selection==i?white:muted);
  if(i==1)menu_text(480,y,g->editor_mode?"EDITOR":"PLAYER",2,white);
  if(i==2)menu_text(480,y,g->automatic_render_distance?"ON":"OFF",2,white);
  if(i==3){char digits[5];int n=(int)g->render_distance;digits[0]=(char)('0'+(n/1000)%10);digits[1]=(char)('0'+(n/100)%10);digits[2]=(char)('0'+(n/10)%10);digits[3]=(char)('0'+n%10);digits[4]=0;menu_text(480,y,digits,2,white);}
  if(i>=4&&i<=6){char value[8];format_tenths(i==4?g->player_speed:(i==5?g->jump_height:g->gravity),value);menu_text(480,y,value,2,white);}
  if(i>=7&&i<=9){char value[8];format_signed_tenths(i==7?g->spawn_point.x:(i==8?g->spawn_point.y:g->spawn_point.z),value);menu_text(480,y,value,2,white);}
  if(i==11)menu_text(480,y,g->save_status==1?"SAVED":(g->save_status==-1?"FAILED":"SLOT 1"),2,white);
  if(i==12)menu_text(480,y,g->save_status==2?"LOADED":(g->save_status==-2?"FAILED":"SLOT 1"),2,white);
  if(i==13){char v[32];const char *nm=owners[g->script_owner_type%5];int n=g->script_owner+1;v[0]=0;strcat(v,nm);if(g->script_owner_type!=BW_OWNER_PLAYER){char num[8];num[0]=' ';num[1]=(char)('0'+(n/10)%10);num[2]=(char)('0'+n%10);num[3]=0;strcat(v,num);}menu_text(480,y,v,2,white);}
  if(i==14)menu_text(480,y,conditions[g->script_condition%12],2,white);
  if(i==15)menu_text(480,y,actions[g->script_action%12],2,white);
  if(i==16)menu_text(480,y,targets[g->script_target_type%5],2,white);
  if(i==17){char num[5];int n=g->script_target_index+1;num[0]=(char)('0'+(n/100)%10);num[1]=(char)('0'+(n/10)%10);num[2]=(char)('0'+n%10);num[3]=0;menu_text(480,y,num,2,white);}
  if(i==18)menu_text(480,y,summons[g->script_summon_type%4],2,white);
  if(i==19){char num[5];int n=g->rule_count;num[0]=(char)('0'+(n/100)%10);num[1]=(char)('0'+(n/10)%10);num[2]=(char)('0'+n%10);num[3]=0;menu_text(480,y,num,2,white);}\n  if(i==20){int count=0;for(int q=0;q<g->rule_count;++q)if(g->rules[q].active&&g->rules[q].owner_type==g->script_owner_type&&g->rules[q].owner==g->script_owner)++count;char num[12];int n=g->script_rule_index+1;num[0]=(char)('0'+(n/10)%10);num[1]=(char)('0'+n%10);num[2]='/';num[3]=(char)('0'+(count/10)%10);num[4]=(char)('0'+count%10);num[5]=0;menu_text(480,y,count?num:"NONE",2,white);}
 }
 menu_text(790,142,"CONTROLS",3,accent);menu_text(790,180,"DPAD UP/DOWN SELECT",2,white);menu_text(790,206,"LEFT/RIGHT CHANGE",2,white);menu_text(790,232,"A CONFIRM  B RESUME",2,white);
 menu_text(790,282,"EDITOR",2,accent);menu_text(790,308,"LOOK AT OBJECT + Y",2,white);menu_text(790,334,"OPEN ITS SCRIPTS",2,white);menu_text(790,360,"Y ALSO CLOSES MENU",2,white);
 menu_text(790,410,"RULE EDITING",2,accent);menu_text(790,436,"SELECT RULE TO EDIT",2,white);menu_text(790,462,"UPDATE OR DELETE IT",2,white);menu_text(790,510,"IF ON USE -> WEAPON",2,white);menu_text(790,536,"IF TIMER -> SUMMON",2,white);
 menu_text(190,652,"ADD NEW RULE / SELECT EXISTING RULE / UPDATE OR DELETE SELECTED RULE",2,muted);
 D3DVIEWPORT9 full;full.X=0;full.Y=0;full.Width=SW;full.Height=SH;full.MinZ=0;full.MaxZ=1;
 dev->SetViewport(&full);dev->SetRenderState(D3DRS_ZENABLE,FALSE);dev->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
 if(menu_n)dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST,menu_n/3,menu_v,sizeof(Vtx));
 dev->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);dev->SetRenderState(D3DRS_ZENABLE,TRUE);
}

static const char *vs_source =
 "struct VS_IN { float4 position : POSITION0; float4 color : COLOR0; };"
 "struct VS_OUT { float4 position : POSITION0; float4 color : COLOR0; };"
 "VS_OUT main(VS_IN input) {"
 " VS_OUT output;"
 " output.position = input.position;"
 " output.color = input.color;"
 " return output;"
 "}";

static const char *ps_source =
 "struct PS_IN { float4 color : COLOR0; };"
 "float4 main(PS_IN input) : COLOR0 { return input.color; }";

static BwVec3 v3(float x,float y,float z){return BwVec3(x,y,z);}
static void put(Vtx *v,BwVec3 p,DWORD c){v->x=p.x;v->y=p.y;v->z=p.z;v->w=1.0f;v->color=c;}
static void line(BwVec3 a,BwVec3 b,DWORD c){if(nl+2>MAX_LINE)return;put(&lines[nl++],a,c);put(&lines[nl++],b,c);}
static void tri(BwVec3 a,BwVec3 b,BwVec3 c,DWORD k){if(nt+3>MAX_TRI)return;put(&tris[nt++],a,k);put(&tris[nt++],b,k);put(&tris[nt++],c,k);}
static void quad(BwVec3 a,BwVec3 b,BwVec3 c,BwVec3 d,DWORD k){tri(a,b,c,k);tri(a,c,d,k);}

static void box(BwVec3 c,BwVec3 h,DWORD k){
 BwVec3 v[8]={
  v3(c.x-h.x,c.y-h.y,c.z-h.z),v3(c.x+h.x,c.y-h.y,c.z-h.z),
  v3(c.x+h.x,c.y+h.y,c.z-h.z),v3(c.x-h.x,c.y+h.y,c.z-h.z),
  v3(c.x-h.x,c.y-h.y,c.z+h.z),v3(c.x+h.x,c.y-h.y,c.z+h.z),
  v3(c.x+h.x,c.y+h.y,c.z+h.z),v3(c.x-h.x,c.y+h.y,c.z+h.z)
 };
 quad(v[0],v[1],v[2],v[3],k);quad(v[5],v[4],v[7],v[6],k);
 quad(v[4],v[0],v[3],v[7],k);quad(v[1],v[5],v[6],v[2],k);
 quad(v[3],v[2],v[6],v[7],k);quad(v[4],v[5],v[1],v[0],k);
 const int e[12][2]={{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
 for(int i=0;i<12;++i)line(v[e[i][0]],v[e[i][1]],D3DCOLOR_ARGB(255,22,28,35));
}
static void wire_box(BwVec3 c,BwVec3 h,DWORD color){
 BwVec3 v[8]={
  v3(c.x-h.x,c.y-h.y,c.z-h.z),v3(c.x+h.x,c.y-h.y,c.z-h.z),
  v3(c.x+h.x,c.y+h.y,c.z-h.z),v3(c.x-h.x,c.y+h.y,c.z-h.z),
  v3(c.x-h.x,c.y-h.y,c.z+h.z),v3(c.x+h.x,c.y-h.y,c.z+h.z),
  v3(c.x+h.x,c.y+h.y,c.z+h.z),v3(c.x-h.x,c.y+h.y,c.z+h.z)
 };
 const int e[12][2]={{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
 for(int i=0;i<12;++i)line(v[e[i][0]],v[e[i][1]],color);
}
static void spike_mesh(const BwBlock *b){
 float x=b->position.x,z=b->position.z,base=b->position.y-b->half;
 BwVec3 a=v3(x-b->half,base,z-b->half),c=v3(x+b->half,base,z-b->half);
 BwVec3 d=v3(x+b->half,base,z+b->half),e=v3(x-b->half,base,z+b->half),tip=v3(x,base+1.0f,z);
 DWORD red=D3DCOLOR_ARGB(255,185,48,48),dark=D3DCOLOR_ARGB(255,65,25,25);
 tri(a,c,tip,red);tri(c,d,tip,red);tri(d,e,tip,red);tri(e,a,tip,red);
 line(a,c,dark);line(c,d,dark);line(d,e,dark);line(e,a,dark);
 line(a,tip,dark);line(c,tip,dark);line(d,tip,dark);line(e,tip,dark);
}


static bool make_shaders(){
 ID3DXBuffer *code=0,*errors=0;
 HRESULT hr=D3DXCompileShader(vs_source,(UINT)strlen(vs_source),0,0,"main","vs_2_0",0,&code,&errors,0);
 if(errors){OutputDebugStringA((const char*)errors->GetBufferPointer());errors->Release();errors=0;}
 if(FAILED(hr)||!code)return false;
 hr=dev->CreateVertexShader((const DWORD*)code->GetBufferPointer(),&vertex_shader);
 code->Release();code=0;
 if(FAILED(hr))return false;

 hr=D3DXCompileShader(ps_source,(UINT)strlen(ps_source),0,0,"main","ps_2_0",0,&code,&errors,0);
 if(errors){OutputDebugStringA((const char*)errors->GetBufferPointer());errors->Release();errors=0;}
 if(FAILED(hr)||!code)return false;
 hr=dev->CreatePixelShader((const DWORD*)code->GetBufferPointer(),&pixel_shader);
 code->Release();code=0;
 if(FAILED(hr))return false;

 static const D3DVERTEXELEMENT9 decl[]={
  {0,0,D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},
  {0,16,D3DDECLTYPE_D3DCOLOR,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_COLOR,0},
  D3DDECL_END()
 };
 hr=dev->CreateVertexDeclaration(decl,&vertex_decl);
 if(FAILED(hr))return false;
 dev->SetVertexShader(vertex_shader);
 dev->SetPixelShader(pixel_shader);
 dev->SetVertexDeclaration(vertex_decl);
 return true;
}

static void scene(const BwGame *g,int slot,int detail){
 nt=nl=0;
 const BwPlayer *cam=&g->players[slot];
 int radius=(int)g->render_distance;
 if(radius>256)radius=256;
 if(radius<24)radius=24;
 if(g->show_grid)for(int i=-radius;i<=radius;i+=detail){
  float grid=(float)i+0.5f;
  line(v3(grid,0.01f,-(float)radius),v3(grid,0.01f,(float)radius),D3DCOLOR_ARGB(255,45,57,70));
  line(v3(-(float)radius,0.01f,grid),v3((float)radius,0.01f,grid),D3DCOLOR_ARGB(255,45,57,70));
 }
 for(int i=0;i<g->block_count;++i){
  const BwBlock *b=&g->blocks[i];if(b->color==0)continue;
  float x=b->position.x-cam->position.x,y=b->position.y-cam->position.y,z=b->position.z-cam->position.z;
  if(x*x+y*y+z*z<=g->render_distance*g->render_distance){
   if(b->spike)spike_mesh(b);else box(b->position,v3(b->half,b->half,b->half),b->color);
  }
 }
 for(int q=0;q<64;++q)if(g->projectiles[q].active){BwVec3 p=g->projectiles[q].position;box(p,v3(.10f,.10f,.10f),D3DCOLOR_ARGB(255,255,190,45));}
 for(int q=0;q<g->pickup_count;++q)if(g->pickups[q].active)box(g->pickups[q].position,v3(.22f,.22f,.22f),g->pickups[q].type==1?D3DCOLOR_ARGB(255,60,170,255):D3DCOLOR_ARGB(255,210,210,220));
 for(int q=0;q<g->prop_count;++q)if(g->props[q].active)box(g->props[q].position,v3(g->props[q].half,g->props[q].half,g->props[q].half),g->props[q].color);
 for(int q=0;q<g->npc_count;++q)if(g->npcs[q].active){const BwNpc *a=&g->npcs[q];DWORD c=a->color?a->color:(a->type==0?D3DCOLOR_ARGB(255,70,210,120):(a->type==1?D3DCOLOR_ARGB(255,220,75,65):D3DCOLOR_ARGB(255,210,145,55)));box(v3(a->position.x,a->position.y+.85f,a->position.z),v3(.32f,.85f,.32f),c);box(v3(a->position.x,a->position.y+1.85f,a->position.z),v3(.24f,.24f,.24f),D3DCOLOR_ARGB(255,210,180,145));}
 for(int i=0;i<g->player_count;++i){
  int s=g->active_slots[i];
  /* The active camera is positioned at eye height inside its own body.
     Drawing that body from the inside creates a huge blue wall in first person. */
  if(s==slot)continue;
  const BwPlayer *p=&g->players[s];
  float feet=p->position.y-BW_EYE_HEIGHT;
  box(v3(p->position.x,feet+BW_PLAYER_HEIGHT*.5f,p->position.z),
      v3(BW_PLAYER_RADIUS,BW_PLAYER_HEIGHT*.5f,BW_PLAYER_RADIUS),
      D3DCOLOR_ARGB(255,75+s*35,145,235));
  box(v3(p->position.x,feet+BW_PLAYER_HEIGHT+.12f,p->position.z),
      v3(.23f,.23f,.23f),D3DCOLOR_ARGB(255,220,185,145));
 }
 BwVec3 flag_base(g->spawn_point.x,0.02f,g->spawn_point.z);
 line(flag_base,v3(flag_base.x,2.5f,flag_base.z),D3DCOLOR_ARGB(255,220,220,225));
 quad(v3(flag_base.x,2.35f,flag_base.z),v3(flag_base.x+0.85f,2.15f,flag_base.z),v3(flag_base.x+0.85f,1.65f,flag_base.z),v3(flag_base.x,1.85f,flag_base.z),D3DCOLOR_ARGB(255,220,55,55));
 if(!g->editor_mode){
  float cp=cosf(cam->pitch),fx=sinf(cam->yaw)*cp,fy=sinf(cam->pitch),fz=cosf(cam->yaw)*cp;
  float sx=cosf(cam->yaw),sz=-sinf(cam->yaw);
  BwVec3 hand=v3(cam->position.x+fx*.48f+sx*.24f,cam->position.y+fy*.48f-.24f,cam->position.z+fz*.48f+sz*.24f);
  float swing=cam->weapon_anim>0.0f?(0.28f-cam->weapon_anim)/0.28f:0.0f;
  if(cam->selected_weapon==2&&cam->has_sword){BwVec3 tip=v3(hand.x+fx*(.72f+sinf(swing*3.14159f)*.35f),hand.y+fy*.72f+cosf(swing*3.14159f)*.22f,hand.z+fz*(.72f+sinf(swing*3.14159f)*.35f));line(hand,tip,D3DCOLOR_ARGB(255,205,215,225));line(v3(hand.x-sx*.09f,hand.y,hand.z-sz*.09f),v3(hand.x+sx*.09f,hand.y,hand.z+sz*.09f),D3DCOLOR_ARGB(255,125,75,40));}
  else if(cam->selected_weapon==1&&cam->has_blaster){float recoil=cam->weapon_anim>0.0f?sinf(swing*3.14159f)*.18f:0.0f;BwVec3 gun=v3(hand.x-fx*recoil,hand.y-fy*recoil,hand.z-fz*recoil);box(gun,v3(.13f,.10f,.22f),D3DCOLOR_ARGB(255,65,75,90));line(v3(gun.x+fx*.20f,gun.y,gun.z+fz*.20f),v3(gun.x+fx*.52f,gun.y,gun.z+fz*.52f),D3DCOLOR_ARGB(255,180,190,205));}
 }
 if(g->editor_mode){
  float fx=sinf(cam->yaw),fz=cosf(cam->yaw);
  BwVec3 preview(cam->position.x+fx*4.0f,cam->position.y-0.5f,cam->position.z+fz*4.0f);
  preview.x=floorf(preview.x+0.5f);preview.y=floorf(preview.y+0.5f)+0.5f;preview.z=floorf(preview.z+0.5f);
  const BwPlayer *ep=&g->players[slot];
  if(ep->hotbar_page==0)wire_box(preview,v3(.5f,.5f,.5f),bw_slot_colors[ep->selected_slot]);
  else if(ep->selected_item==0){BwBlock spike(preview,0.5f,D3DCOLOR_ARGB(255,190,65,65));spike.spike=true;spike_mesh(&spike);}
  else if(ep->selected_item==3){BwVec3 base(preview.x,0.02f,preview.z);line(base,v3(base.x,2.5f,base.z),D3DCOLOR_ARGB(255,220,220,225));quad(v3(base.x,2.35f,base.z),v3(base.x+.85f,2.15f,base.z),v3(base.x+.85f,1.65f,base.z),v3(base.x,1.85f,base.z),D3DCOLOR_ARGB(255,220,55,55));}
  else if(ep->selected_item==4||ep->selected_item==5||ep->selected_item==6)wire_box(v3(preview.x,.85f,preview.z),v3(.32f,.85f,.32f),ep->selected_item==4?D3DCOLOR_ARGB(255,70,210,120):(ep->selected_item==5?D3DCOLOR_ARGB(255,220,75,65):D3DCOLOR_ARGB(255,210,145,55)));
  else wire_box(preview,v3(.25f,.25f,.25f),D3DCOLOR_ARGB(255,40,230,255));
 }
}

/* Transform world-space vertices on the CPU using the XDK's D3DX matrix
   implementation. The GPU shader is deliberately pass-through: it receives
   valid homogeneous clip-space coordinates and only interpolates vertex color. */
static void transform_vertices(Vtx *v,int count,const D3DXMATRIX *m){
 for(int i=0;i<count;++i){
  D3DXVECTOR4 in,out;
  in.x=v[i].x;in.y=v[i].y;in.z=v[i].z;in.w=v[i].w;
  D3DXVec4Transform(&out,&in,m);
  v[i].x=out.x;v[i].y=out.y;v[i].z=out.z;v[i].w=out.w;
 }
}

static D3DXMATRIX camera_matrix(const BwPlayer *p,float aspect,float far_plane){
 D3DXVECTOR3 eye,at,up;
 float cp=cosf(p->pitch);
 eye.x=p->position.x;eye.y=p->position.y;eye.z=p->position.z;
 at.x=eye.x+sinf(p->yaw)*cp;
 at.y=eye.y+sinf(p->pitch);
 at.z=eye.z+cosf(p->yaw)*cp;
 up.x=0.0f;up.y=1.0f;up.z=0.0f;
 D3DXMATRIX view,projection,result;
 D3DXMatrixLookAtLH(&view,&eye,&at,&up);
 D3DXMatrixPerspectiveFovLH(&projection,PI/3.0f,aspect,0.1f,far_plane);
 D3DXMatrixMultiply(&result,&view,&projection);
 return result;
}

static void hudline(Vtx *v,int *n,float x,float y,float X,float Y,DWORD c){
 if(*n+2>256)return;
 v[*n].x=(x/(float)SW)*2.0f-1.0f;v[*n].y=1.0f-(y/(float)SH)*2.0f;v[*n].z=0;v[*n].w=1;v[*n].color=c;++*n;
 v[*n].x=(X/(float)SW)*2.0f-1.0f;v[*n].y=1.0f-(Y/(float)SH)*2.0f;v[*n].z=0;v[*n].w=1;v[*n].color=c;++*n;
}
static void hudrect(Vtx *v,int *n,float x,float y,float w,float h,DWORD c){
 hudline(v,n,x,y,x+w,y,c);hudline(v,n,x+w,y,x+w,y+h,c);
 hudline(v,n,x+w,y+h,x,y+h,c);hudline(v,n,x,y+h,x,y,c);
}
static void hud(const BwGame *g,int slot,View vp){
 Vtx v[256];int n=0;
 float s=1.0f,w=34*s,h=30*s,gap=4*s;
 float total=8*w+7*gap,start=vp.x+(vp.w-total)*.5f,y=vp.y+vp.h-h-18*s;
 DWORD mode=g->editor_mode?D3DCOLOR_ARGB(255,235,165,65):D3DCOLOR_ARGB(255,90,190,255);
 const BwPlayer *p=&g->players[slot];
 static const char *item_names[8]={"SPIKE","BLAST","SWORD","FLAG","FRIEND","SWORD","GUN","CRATE"};
 for(int i=0;i<8;++i){float x=start+i*(w+gap);bool active=p->hotbar_page==0?(i==p->selected_slot):(i==p->selected_item);hudrect(v,&n,x,y,w,h,active?mode:D3DCOLOR_ARGB(255,125,135,150));if(p->hotbar_page==0)hudrect(v,&n,x+9*s,y+8*s,16*s,14*s,bw_slot_colors[i]);}
 D3DVIEWPORT9 full;full.X=0;full.Y=0;full.Width=SW;full.Height=SH;full.MinZ=0;full.MaxZ=1;
 dev->SetViewport(&full);
 dev->SetRenderState(D3DRS_ZENABLE,FALSE);
 dev->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
 if(n)dev->DrawPrimitiveUP(D3DPT_LINELIST,n/2,v,sizeof(Vtx));
 menu_n=0;if(p->hotbar_page){static const char *short_names[8]={"SPK","BLS","SWD","FLG","FRI","ENM","GUN","BOX"};for(int i=0;i<8;++i)menu_text(start+i*(w+gap)+8.0f,y+10.0f,short_names[i],1,p->selected_item==i?D3DCOLOR_ARGB(255,255,240,150):D3DCOLOR_ARGB(255,190,205,220));}const char *nm=p->hotbar_page?(item_names[p->selected_item]):(g->editor_mode?"BLOCK COLORS":(p->selected_weapon==0?"EMPTY":(p->selected_weapon==1?"BLASTER":(p->selected_weapon==2?"SWORD":"ITEM"))));menu_text(vp.x+18.0f,vp.y+18.0f,nm,2,D3DCOLOR_ARGB(255,255,230,150));
 if(!g->editor_mode&&(p->selected_weapon==1||p->selected_weapon==2)){int d=(int)(p->selected_weapon==1?p->blaster_durability:p->sword_durability);char digits[4];digits[0]=(char)('0'+(d/100)%10);digits[1]=(char)('0'+(d/10)%10);digits[2]=(char)('0'+d%10);digits[3]=0;menu_text(vp.x+18.0f,vp.y+36.0f,digits,2,D3DCOLOR_ARGB(255,230,230,230));}
 if(menu_n)dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST,menu_n/3,menu_v,sizeof(Vtx));
 dev->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
 dev->SetRenderState(D3DRS_ZENABLE,TRUE);
}

bool bw_renderer_open(void){
 d3d=Direct3DCreate9(D3D_SDK_VERSION);
 if(!d3d)return false;
 D3DPRESENT_PARAMETERS p;ZeroMemory(&p,sizeof(p));
 p.BackBufferWidth=SW;p.BackBufferHeight=SH;p.BackBufferFormat=D3DFMT_X8R8G8B8;
 p.BackBufferCount=2;p.MultiSampleType=D3DMULTISAMPLE_NONE;
 p.SwapEffect=D3DSWAPEFFECT_DISCARD;p.hDeviceWindow=0;p.Windowed=FALSE;
 p.EnableAutoDepthStencil=TRUE;p.AutoDepthStencilFormat=D3DFMT_D24S8;
 p.FullScreen_RefreshRateInHz=60;p.PresentationInterval=D3DPRESENT_INTERVAL_ONE;
 HRESULT hr=d3d->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,0,
  D3DCREATE_HARDWARE_VERTEXPROCESSING,&p,&dev);
 if(FAILED(hr)){d3d->Release();d3d=0;return false;}
 dev->SetRenderState(D3DRS_ZENABLE,TRUE);
 dev->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
 dev->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
 dev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
 dev->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
 if(!make_shaders()){bw_renderer_close();return false;}
 return true;
}

void bw_renderer_draw(const BwGame *g){
 if(!dev)return;
 if(g->paused){
  dev->Clear(0,0,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,D3DCOLOR_ARGB(255,8,13,22),1.0f,0);
  if(SUCCEEDED(dev->BeginScene())){controls_menu(g);dev->EndScene();dev->Present(0,0,0,0);}
  return;
 }
 int detail=g->quality_scale>.85f?1:(g->quality_scale>.70f?2:3);
 dev->Clear(0,0,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,D3DCOLOR_ARGB(255,16,22,32),1.0f,0);
 if(FAILED(dev->BeginScene()))return;

 /* Every joined controller receives its own view. */
 int view_count=g->player_count>1?g->player_count:1;
 for(int i=0;i<view_count;++i){
  int slot=(view_count>1)?g->active_slots[i]:g->active_slots[0];
  View vp;vp.x=vp.y=0;vp.w=SW;vp.h=SH;
  if(view_count==2){vp.x=i*(SW/2);vp.w=SW/2;}
  else if(view_count>=3){vp.x=(i%2)*(SW/2);vp.y=(i/2)*(SH/2);vp.w=SW/2;vp.h=SH/2;}
  scene(g,slot,detail);
  D3DVIEWPORT9 nv;nv.X=vp.x;nv.Y=vp.y;nv.Width=vp.w;nv.Height=vp.h;nv.MinZ=0;nv.MaxZ=1;
  dev->SetViewport(&nv);
  dev->SetRenderState(D3DRS_ZENABLE,TRUE);
  dev->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
  D3DXMATRIX mvp=camera_matrix(&g->players[slot],(float)vp.w/(float)vp.h,g->render_distance);
  transform_vertices(tris,nt,&mvp);
  transform_vertices(lines,nl,&mvp);
  if(nt)dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST,nt/3,tris,sizeof(Vtx));
  if(nl)dev->DrawPrimitiveUP(D3DPT_LINELIST,nl/2,lines,sizeof(Vtx));
  hud(g,slot,vp);
 }
 dev->EndScene();
 dev->Present(0,0,0,0);
}

void bw_renderer_close(void){
 if(dev){
  if(vertex_decl){vertex_decl->Release();vertex_decl=0;}
  if(pixel_shader){pixel_shader->Release();pixel_shader=0;}
  if(vertex_shader){vertex_shader->Release();vertex_shader=0;}
  dev->Release();dev=0;
 }
 if(d3d){d3d->Release();d3d=0;}
}
