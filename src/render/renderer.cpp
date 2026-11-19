#include "game/game.h"
#include <math.h>
static const int SW=1280,SH=720,MAX_TRI=24576,MAX_LINE=8192;
static const float PI=3.14159265358979323846f;
struct Vtx{float x,y,z;DWORD color;};
struct HudVtx{float x,y,z,rhw;DWORD color;};
struct View{int x,y,w,h;};
#define WORLD_FVF (D3DFVF_XYZ|D3DFVF_DIFFUSE)
#define HUD_FVF (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)
static IDirect3D9 *d3d=0;static IDirect3DDevice9 *dev=0;
static Vtx tris[MAX_TRI],lines[MAX_LINE];static int nt=0,nl=0;
static BwVec3 v3(float x,float y,float z){return BwVec3(x,y,z);}
static void put(Vtx *v,BwVec3 p,DWORD c){v->x=p.x;v->y=p.y;v->z=p.z;v->color=c;}
static void line(BwVec3 a,BwVec3 b,DWORD c){if(nl+2>MAX_LINE)return;put(&lines[nl++],a,c);put(&lines[nl++],b,c);}
static void tri(BwVec3 a,BwVec3 b,BwVec3 c,DWORD k){if(nt+3>MAX_TRI)return;put(&tris[nt++],a,k);put(&tris[nt++],b,k);put(&tris[nt++],c,k);}
static void quad(BwVec3 a,BwVec3 b,BwVec3 c,BwVec3 d,DWORD k){tri(a,b,c,k);tri(a,c,d,k);}
static void box(BwVec3 c,BwVec3 h,DWORD k){
 BwVec3 v[8]={v3(c.x-h.x,c.y-h.y,c.z-h.z),v3(c.x+h.x,c.y-h.y,c.z-h.z),v3(c.x+h.x,c.y+h.y,c.z-h.z),v3(c.x-h.x,c.y+h.y,c.z-h.z),v3(c.x-h.x,c.y-h.y,c.z+h.z),v3(c.x+h.x,c.y-h.y,c.z+h.z),v3(c.x+h.x,c.y+h.y,c.z+h.z),v3(c.x-h.x,c.y+h.y,c.z+h.z)};
 quad(v[0],v[1],v[2],v[3],k);quad(v[5],v[4],v[7],v[6],k);quad(v[4],v[0],v[3],v[7],k);quad(v[1],v[5],v[6],v[2],k);quad(v[3],v[2],v[6],v[7],k);quad(v[4],v[5],v[1],v[0],k);
 const int e[12][2]={{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
 for(int i=0;i<12;++i)line(v[e[i][0]],v[e[i][1]],D3DCOLOR_ARGB(255,22,28,35));
}
static D3DMATRIX ident(){D3DMATRIX m;ZeroMemory(&m,sizeof(m));m._11=m._22=m._33=m._44=1;return m;}
static D3DMATRIX look(BwVec3 eye,BwVec3 at){
 BwVec3 z=v3(at.x-eye.x,at.y-eye.y,at.z-eye.z);float n=sqrtf(z.x*z.x+z.y*z.y+z.z*z.z);if(n<.0001f)n=1;z.x/=n;z.y/=n;z.z/=n;
 BwVec3 x=v3(z.z,0,-z.x);n=sqrtf(x.x*x.x+x.z*x.z);if(n<.0001f)n=1;x.x/=n;x.z/=n;BwVec3 y=v3(-z.y*x.z,z.z*x.x-z.x*x.z,z.y*x.x);D3DMATRIX m;
 m._11=x.x;m._12=y.x;m._13=z.x;m._14=0;m._21=x.y;m._22=y.y;m._23=z.y;m._24=0;m._31=x.z;m._32=y.z;m._33=z.z;m._34=0;m._41=-(x.x*eye.x+x.y*eye.y+x.z*eye.z);m._42=-(y.x*eye.x+y.y*eye.y+y.z*eye.z);m._43=-(z.x*eye.x+z.y*eye.y+z.z*eye.z);m._44=1;return m;
}
static D3DMATRIX persp(float f,float a,float zn,float zf){D3DMATRIX m;ZeroMemory(&m,sizeof(m));float y=1/tanf(f*.5f);m._11=y/a;m._22=y;m._33=zf/(zf-zn);m._34=1;m._43=-zn*zf/(zf-zn);return m;}
static void scene(const BwGame *g,int slot,int detail){
 nt=nl=0;const BwPlayer *cam=&g->players[slot];int radius=g->player_count==1?12:(g->player_count==2?8:5);
 if(g->show_grid)for(int i=-radius;i<=radius;i+=detail){line(v3((float)i,0,-(float)radius),v3((float)i,0,(float)radius),D3DCOLOR_ARGB(255,45,57,70));line(v3(-(float)radius,0,(float)i),v3((float)radius,0,(float)i),D3DCOLOR_ARGB(255,45,57,70));}
 for(int i=0;i<g->block_count;++i){const BwBlock *b=&g->blocks[i];float x=b->position.x-cam->position.x,y=b->position.y-cam->position.y,z=b->position.z-cam->position.z;if(x*x+y*y+z*z<=g->render_distance*g->render_distance)box(b->position,v3(b->half,b->half,b->half),b->color);}
 for(int i=0;i<g->player_count;++i){int s=g->active_slots[i];const BwPlayer *p=&g->players[s];float feet=p->position.y-BW_EYE_HEIGHT;box(v3(p->position.x,feet+BW_PLAYER_HEIGHT*.5f,p->position.z),v3(BW_PLAYER_RADIUS,BW_PLAYER_HEIGHT*.5f,BW_PLAYER_RADIUS),D3DCOLOR_ARGB(255,75+s*35,145,235));box(v3(p->position.x,feet+BW_PLAYER_HEIGHT+.12f,p->position.z),v3(.23f,.23f,.23f),D3DCOLOR_ARGB(255,220,185,145));}
}
static void hudline(HudVtx *v,int *n,float x,float y,float X,float Y,DWORD c){if(*n+2>256)return;v[*n].x=x;v[*n].y=y;v[*n].z=0;v[*n].rhw=1;v[*n].color=c;++*n;v[*n].x=X;v[*n].y=Y;v[*n].z=0;v[*n].rhw=1;v[*n].color=c;++*n;}
static void hudrect(HudVtx *v,int *n,float x,float y,float w,float h,DWORD c){hudline(v,n,x,y,x+w,y,c);hudline(v,n,x+w,y,x+w,y+h,c);hudline(v,n,x+w,y+h,x,y+h,c);hudline(v,n,x,y+h,x,y,c);}
static void hud(const BwGame *g,int slot,View vp){
 HudVtx v[256];int n=0;float s=g->player_count==1?1.0f:.72f,w=34*s,h=30*s,gap=4*s,total=8*w+7*gap,start=vp.x+(vp.w-total)*.5f,y=vp.y+vp.h-h-18*s;DWORD mode=g->editor_mode?D3DCOLOR_ARGB(255,235,165,65):D3DCOLOR_ARGB(255,90,190,255);
 for(int i=0;i<8;++i){float x=start+i*(w+gap);hudrect(v,&n,x,y,w,h,i==g->players[slot].selected_slot?mode:D3DCOLOR_ARGB(255,125,135,150));hudrect(v,&n,x+9*s,y+8*s,16*s,14*s,bw_slot_colors[i]);}
 D3DVIEWPORT9 full;full.X=0;full.Y=0;full.Width=SW;full.Height=SH;full.MinZ=0;full.MaxZ=1;dev->SetViewport(&full);D3DMATRIX id=ident();dev->SetTransform(D3DTS_WORLD,&id);dev->SetTransform(D3DTS_VIEW,&id);dev->SetTransform(D3DTS_PROJECTION,&id);dev->SetVertexShader(0);dev->SetPixelShader(0);dev->SetFVF(HUD_FVF);if(n)dev->DrawPrimitiveUP(D3DPT_LINELIST,n/2,v,sizeof(HudVtx));
}
bool bw_renderer_open(void){
 d3d=Direct3DCreate9(D3D_SDK_VERSION);if(!d3d)return false;D3DPRESENT_PARAMETERS p;ZeroMemory(&p,sizeof(p));p.BackBufferWidth=SW;p.BackBufferHeight=SH;p.BackBufferFormat=D3DFMT_X8R8G8B8;p.BackBufferCount=2;p.MultiSampleType=D3DMULTISAMPLE_NONE;p.SwapEffect=D3DSWAPEFFECT_DISCARD;p.hDeviceWindow=0;p.Windowed=FALSE;p.EnableAutoDepthStencil=TRUE;p.AutoDepthStencilFormat=D3DFMT_D24S8;p.FullScreen_RefreshRateInHz=60;p.PresentationInterval=D3DPRESENT_INTERVAL_ONE;
 HRESULT hr=d3d->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,0,D3DCREATE_HARDWARE_VERTEXPROCESSING,&p,&dev);if(FAILED(hr)){d3d->Release();d3d=0;return false;}dev->SetRenderState(D3DRS_LIGHTING,FALSE);dev->SetRenderState(D3DRS_ZENABLE,TRUE);dev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);dev->SetFVF(WORLD_FVF);return true;
}
void bw_renderer_draw(const BwGame *g){
 if(!dev)return;int detail=g->quality_scale>.85f?1:(g->quality_scale>.70f?2:3);dev->Clear(0,0,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,D3DCOLOR_ARGB(255,16,22,32),1,0);dev->BeginScene();
 for(int i=0;i<g->player_count;++i){int slot=g->active_slots[i];View vp;vp.x=vp.y=0;vp.w=SW;vp.h=SH;if(g->player_count==2){vp.x=i*(SW/2);vp.w=SW/2;}else if(g->player_count>=3){vp.x=(i%2)*(SW/2);vp.y=(i/2)*(SH/2);vp.w=SW/2;vp.h=SH/2;}
 scene(g,slot,detail);D3DVIEWPORT9 nv;nv.X=vp.x;nv.Y=vp.y;nv.Width=vp.w;nv.Height=vp.h;nv.MinZ=0;nv.MaxZ=1;dev->SetViewport(&nv);const BwPlayer *p=&g->players[slot];float cp=cosf(p->pitch);BwVec3 at=v3(p->position.x+sinf(p->yaw)*cp,p->position.y+sinf(p->pitch),p->position.z+cosf(p->yaw)*cp);
 D3DMATRIX world=ident(),view=look(p->position,at),proj=persp(PI/3,(float)vp.w/vp.h,.1f,g->render_distance);dev->SetTransform(D3DTS_WORLD,&world);dev->SetTransform(D3DTS_VIEW,&view);dev->SetTransform(D3DTS_PROJECTION,&proj);dev->SetVertexShader(0);dev->SetPixelShader(0);dev->SetFVF(WORLD_FVF);
 if(nt)dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST,nt/3,tris,sizeof(Vtx));if(nl)dev->DrawPrimitiveUP(D3DPT_LINELIST,nl/2,lines,sizeof(Vtx));hud(g,slot,vp);
 }
 dev->EndScene();dev->Present(0,0,0,0);
}
void bw_renderer_close(void){if(dev){dev->Release();dev=0;}if(d3d){d3d->Release();d3d=0;}}
