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
 int radius=g->player_count==1?12:(g->player_count==2?8:5);
 if(g->show_grid)for(int i=-radius;i<=radius;i+=detail){
  line(v3((float)i,0,-(float)radius),v3((float)i,0,(float)radius),D3DCOLOR_ARGB(255,45,57,70));
  line(v3(-(float)radius,0,(float)i),v3((float)radius,0,(float)i),D3DCOLOR_ARGB(255,45,57,70));
 }
 for(int i=0;i<g->block_count;++i){
  const BwBlock *b=&g->blocks[i];
  float x=b->position.x-cam->position.x,y=b->position.y-cam->position.y,z=b->position.z-cam->position.z;
  if(x*x+y*y+z*z<=g->render_distance*g->render_distance)
   box(b->position,v3(b->half,b->half,b->half),b->color);
 }
 for(int i=0;i<g->player_count;++i){
  int s=g->active_slots[i];const BwPlayer *p=&g->players[s];
  float feet=p->position.y-BW_EYE_HEIGHT;
  box(v3(p->position.x,feet+BW_PLAYER_HEIGHT*.5f,p->position.z),
      v3(BW_PLAYER_RADIUS,BW_PLAYER_HEIGHT*.5f,BW_PLAYER_RADIUS),
      D3DCOLOR_ARGB(255,75+s*35,145,235));
  box(v3(p->position.x,feet+BW_PLAYER_HEIGHT+.12f,p->position.z),
      v3(.23f,.23f,.23f),D3DCOLOR_ARGB(255,220,185,145));
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
 for(int i=0;i<8;++i){
  float x=start+i*(w+gap);
  hudrect(v,&n,x,y,w,h,i==g->players[slot].selected_slot?mode:D3DCOLOR_ARGB(255,125,135,150));
  hudrect(v,&n,x+9*s,y+8*s,16*s,14*s,bw_slot_colors[i]);
 }
 D3DVIEWPORT9 full;full.X=0;full.Y=0;full.Width=SW;full.Height=SH;full.MinZ=0;full.MaxZ=1;
 dev->SetViewport(&full);
 dev->SetRenderState(D3DRS_ZENABLE,FALSE);
 dev->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
 if(n)dev->DrawPrimitiveUP(D3DPT_LINELIST,n/2,v,sizeof(Vtx));
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
 int detail=g->quality_scale>.85f?1:(g->quality_scale>.70f?2:3);
 dev->Clear(0,0,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,D3DCOLOR_ARGB(255,16,22,32),1.0f,0);
 if(FAILED(dev->BeginScene()))return;

 /* Split-screen is opt-in. Multiple connected pads alone never divide the TV. */
 int view_count=(g->split_screen&&g->player_count>1)?g->player_count:1;
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
