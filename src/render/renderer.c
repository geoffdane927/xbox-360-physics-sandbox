#include "render/renderer.h"
#include <math.h>
#include <string.h>
#include "blockworks360_shaders.h"
#include "xsdk/gpu.h"
#include "xsdk/kernel.h"
#include "xsdk/log.h"
#include "xsdk/present.h"
#include "xsdk/shader.h"
#include "xsdk/display.h"

#define BW_MAX_VERTICES 24000
#define BW_VERTEX_WORDS 7
#define BW_PI 3.14159265358979323846f
#define BW_BEGIN_TIMEOUT_MS 5000
#define BW_CLEAR 0xFF101620

typedef struct { float x, y, z, r, g, b, a; } BwVertex;
typedef struct { float x, y, z; } V3;
static XsdkShader vertex_shader, pixel_shader;
static volatile uint32_t *gpu_vertices;
static BwVertex vertices[BW_MAX_VERTICES];
static uint32_t vertex_count;
static uint32_t screen_w, screen_h;

static V3 sub(V3 a, V3 b) { return (V3){a.x-b.x,a.y-b.y,a.z-b.z}; }
static float dot(V3 a, V3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
static V3 cross(V3 a, V3 b) { return (V3){a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
static V3 norm(V3 a) {
    const float n=sqrtf(dot(a,a));
    return n>0.00001f?(V3){a.x/n,a.y/n,a.z/n}:(V3){0,0,1};
}
static void push_vertex(const BwGame *game, int player_index, V3 point, uint32_t color)
{
    if (vertex_count >= BW_MAX_VERTICES) return;
    const BwPlayer *p=&game->players[player_index];
    const V3 eye={p->position.x,p->position.y+1.55f,p->position.z};
    const V3 forward={sinf(p->yaw)*cosf(p->pitch),sinf(p->pitch),cosf(p->yaw)*cosf(p->pitch)};
    const V3 right=norm(cross((V3){0,1,0},forward));
    const V3 up=cross(forward,right);
    const V3 rel=sub(point,eye);
    const float z=dot(rel,forward);
    if(z<0.10f) return;
    const float aspect=(float)screen_w/(float)screen_h;
    const float scale=1.0f/tanf(BW_PI/6.0f);
    const float nx=dot(rel,right)*scale/(z*aspect);
    const float ny=dot(rel,up)*scale/z;
    BwVertex *v=&vertices[vertex_count++];
    v->x=nx; v->y=ny; v->z=z/160.0f;
    v->r=(float)((color>>16)&255)/255.0f;
    v->g=(float)((color>>8)&255)/255.0f;
    v->b=(float)(color&255)/255.0f;
    v->a=1.0f;
}
static void triangle(const BwGame *g,int pi,V3 a,V3 b,V3 c,uint32_t color)
{
    const uint32_t start=vertex_count;
    push_vertex(g,pi,a,color); push_vertex(g,pi,b,color); push_vertex(g,pi,c,color);
    if(vertex_count!=start+3) vertex_count=start;
}
static void quad(const BwGame *g,int pi,V3 a,V3 b,V3 c,V3 d,uint32_t color)
{
    triangle(g,pi,a,b,c,color); triangle(g,pi,a,c,d,color);
}
static void box(const BwGame *g,int pi,V3 c,V3 h,uint32_t color)
{
    V3 v[8]={
        {c.x-h.x,c.y-h.y,c.z-h.z},{c.x+h.x,c.y-h.y,c.z-h.z},
        {c.x+h.x,c.y+h.y,c.z-h.z},{c.x-h.x,c.y+h.y,c.z-h.z},
        {c.x-h.x,c.y-h.y,c.z+h.z},{c.x+h.x,c.y-h.y,c.z+h.z},
        {c.x+h.x,c.y+h.y,c.z+h.z},{c.x-h.x,c.y+h.y,c.z+h.z}
    };
    quad(g,pi,v[0],v[1],v[2],v[3],color); quad(g,pi,v[5],v[4],v[7],v[6],color);
    quad(g,pi,v[4],v[0],v[3],v[7],color); quad(g,pi,v[1],v[5],v[6],v[2],color);
    quad(g,pi,v[3],v[2],v[6],v[7],color); quad(g,pi,v[4],v[5],v[1],v[0],color);
}
static void build_view(const BwGame *game,int player_index)
{
    vertex_count=0;
    if(game->show_grid) {
        for(int n=-12;n<=12;++n) {
            const float q=(float)n;
            box(game,player_index,(V3){q,0.006f,0.0f},(V3){0.008f,0.006f,12.0f},0xFF394653);
            box(game,player_index,(V3){0.0f,0.006f,q},(V3){12.0f,0.006f,0.008f},0xFF394653);
        }
    }
    for(int i=0;i<game->block_count;++i) {
        const BwBlock *b=&game->blocks[i];
        const BwPlayer *p=&game->players[player_index];
        const float dx=b->position.x-p->position.x,dy=b->position.y-p->position.y,dz=b->position.z-p->position.z;
        if(dx*dx+dy*dy+dz*dz>game->render_distance*game->render_distance) continue;
        box(game,player_index,(V3){b->position.x,b->position.y,b->position.z},
            (V3){b->half,b->half,b->half},b->color);
    }
    for(int i=0;i<game->player_count;++i) {
        const int slot=game->active_slots[i];
        const BwPlayer *p=&game->players[slot];
        box(game,player_index,(V3){p->position.x,p->position.y+0.87f,p->position.z},
            (V3){0.32f,0.87f,0.32f},0xFF4B91EB);
        box(game,player_index,(V3){p->position.x,p->position.y+1.98f,p->position.z},
            (V3){0.23f,0.23f,0.23f},0xFFDDBB91);
    }
}
static void upload_vertices(void)
{
    for(uint32_t i=0;i<vertex_count;++i) {
        uint32_t words[BW_VERTEX_WORDS];
        memcpy(words,&vertices[i],sizeof(BwVertex));
        for(uint32_t j=0;j<BW_VERTEX_WORDS;++j) gpu_vertices[i*BW_VERTEX_WORDS+j]=words[j];
    }
    xsdk_gpu_invalidate(xsdk_gpu_address(gpu_vertices),vertex_count*sizeof(BwVertex));
}

int bw_renderer_open(uint32_t *width,uint32_t *height)
{
    NTSTATUS status=xsdk_gpu_open();
    if(!NT_SUCCESS(status)) return (int)status;
    status=xsdk_shader_load(&vertex_shader,world_vert.container,world_vert.container_bytes);
    if(NT_SUCCESS(status)) status=xsdk_shader_load(&pixel_shader,world_frag.container,world_frag.container_bytes);
    if(!NT_SUCCESS(status)) return (int)status;
    const XsdkVertexElement layout[]={
        {.usage=XSDK_USAGE_POSITION,.format=XSDK_VERTEX_FLOAT3,.offset_bytes=0},
        {.usage=XSDK_USAGE_COLOR,.format=XSDK_VERTEX_FLOAT4,.offset_bytes=12}
    };
    const uint32_t stride[]={sizeof(BwVertex)};
    status=xsdk_shader_set_vertex_layout(&vertex_shader,layout,2,stride);
    if(!NT_SUCCESS(status)) return (int)status;
    gpu_vertices=xsdk_gpu_alloc(BW_MAX_VERTICES*sizeof(BwVertex));
    if(!gpu_vertices) return -1;
    status=xsdk_present_open(XSDK_DISPLAY_720P,3,1,width,height);
    if(!NT_SUCCESS(status)) return (int)status;
    screen_w=*width; screen_h=*height;
    return 0;
}
void bw_renderer_draw(const BwGame *game,uint32_t width,uint32_t height)
{
    uint32_t address=0;
    NTSTATUS status=xsdk_present_begin(&address,BW_BEGIN_TIMEOUT_MS);
    if(!NT_SUCCESS(status)) { xsdk_log_write("renderer: present_begin failed %08X",(unsigned)status); return; }
    const uint32_t depth_tile=xsdk_gpu_target_tiles(width,height);
    xsdk_gpu_set_target_at(width,height,0,depth_tile);
    XsdkDepthStencil depth={0};
    depth.depth_test=true; depth.depth_write=true; depth.depth_compare=XSDK_COMPARE_LESS;
    XsdkDepthStencilState depth_state;
    xsdk_gpu_depth_stencil_state(&depth_state,&depth);
    xsdk_gpu_set_depth_clear(1.0f,0);
    xsdk_gpu_set_depth_stencil(&depth_state);
    xsdk_gpu_set_shaders(&vertex_shader,&pixel_shader);
    xsdk_built_shader_set_literals(&world_vert);
    xsdk_built_shader_set_literals(&world_frag);
    xsdk_gpu_set_vertices(0,xsdk_gpu_address(gpu_vertices),BW_MAX_VERTICES*sizeof(BwVertex));
    for(int view=0;view<game->player_count;++view) {
        const int slot=game->active_slots[view];
        uint32_t x=0,y=0,w=width,h=height;
        if(game->player_count==2) { w=width/2; x=(uint32_t)view*w; }
        else if(game->player_count>=3) { w=width/2; h=height/2; x=(uint32_t)(view%2)*w; y=(uint32_t)(view/2)*h; }
        xsdk_gpu_set_viewport((float)x,(float)y,(float)w,(float)h,0.0f,1.0f);
        xsdk_gpu_set_scissor(x,y,w,h);
        build_view(game,slot);
        upload_vertices();
        if(vertex_count) xsdk_gpu_draw(XSDK_PRIMITIVE_TRIANGLE_LIST,vertex_count);
    }
    xsdk_gpu_resolve(address,width,height,BW_CLEAR);
    xsdk_present_end();
}
void bw_renderer_close(void)
{
    xsdk_present_close();
    if(gpu_vertices) { xsdk_gpu_free(gpu_vertices); gpu_vertices=0; }
    xsdk_shader_free(&pixel_shader);
    xsdk_shader_free(&vertex_shader);
    const NTSTATUS status=xsdk_gpu_close();
    if(!NT_SUCCESS(status)) xsdk_log_write("renderer: gpu_close failed %08X",(unsigned)status);
}
