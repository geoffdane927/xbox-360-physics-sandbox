#include <xtl.h>
#include <d3d9.h>
#include <xinput.h>
#include <math.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "xinput.lib")

// BlockWorks 360 - basic playable graphics / collision test.
// Upright characters use simple movement and gravity. There is no ragdoll yet.

static const int SCREEN_W = 1280;
static const int SCREEN_H = 720;
static const int MAX_TRI_VERTICES = 24576;
static const int MAX_LINE_VERTICES = 8192;
static const int MAX_BLOCKS = 256;
static const int PLAYER_LIMIT = 4;
static const int INVENTORY_SLOTS = 8;
static const DWORD FRAME_CAP_MS = 16; // 60 Hz ceiling; vertical sync is also enabled.
static const float PI_F = 3.14159265358979323846f;
static const float PLAYER_RADIUS = 0.32f;
static const float PLAYER_HEIGHT = 1.75f;
static const float EYE_HEIGHT = 1.55f;
static const float GRAVITY = 18.0f;

struct Vec3
{
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float X, float Y, float Z) : x(X), y(Y), z(Z) {}
};

struct Vertex
{
    float x, y, z;
    DWORD color;
};

struct HudVertex
{
    float x, y, z, rhw;
    DWORD color;
};

#define BW_FVF (D3DFVF_XYZ | D3DFVF_DIFFUSE)
#define BW_HUD_FVF (D3DFVF_XYZRHW | D3DFVF_DIFFUSE)

static IDirect3D9* g_d3d = 0;
static IDirect3DDevice9* g_device = 0;
static Vertex g_triangles[MAX_TRI_VERTICES];
static Vertex g_lines[MAX_LINE_VERTICES];
static int g_triangleCount = 0;
static int g_lineCount = 0;

struct Camera
{
    Vec3 position; // In player mode this is the eye position.
    float yaw;
    float pitch;
};

struct Controller
{
    XINPUT_GAMEPAD current;
    XINPUT_GAMEPAD previous;
    bool connected;
    float lx, ly, rx, ry;
    float leftTrigger, rightTrigger;
    bool pressedA, pressedB, pressedX, pressedY;
    bool pressedBack, pressedStart;
    bool pressedDpadLeft, pressedDpadRight;
};

struct Player
{
    Camera camera;
    float verticalVelocity;
    int selectedSlot;
    bool grounded;
};

struct Block
{
    Vec3 position;
    float halfSize;
    DWORD color;
};

struct Viewport
{
    int x, y, width, height;
};

static Controller g_pads[PLAYER_LIMIT];
static Player g_players[PLAYER_LIMIT];
static Block g_blocks[MAX_BLOCKS];
static int g_blockCount = 0;
static int g_playerCount = 1;
static int g_activePlayers[PLAYER_LIMIT] = {0, 1, 2, 3};
static bool g_editorMode = true;
static bool g_showGrid = true;
static DWORD g_lastTick = 0;
static float g_fpsAverage = 60.0f;
static float g_qualityScale = 1.0f;

static const DWORD g_itemColors[INVENTORY_SLOTS] =
{
    D3DCOLOR_ARGB(255, 90, 205, 155),
    D3DCOLOR_ARGB(255, 90, 160, 235),
    D3DCOLOR_ARGB(255, 220, 170, 75),
    D3DCOLOR_ARGB(255, 190, 100, 210),
    D3DCOLOR_ARGB(255, 190, 195, 205),
    D3DCOLOR_ARGB(255, 210, 100, 90),
    D3DCOLOR_ARGB(255, 100, 190, 210),
    D3DCOLOR_ARGB(255, 145, 175, 100)
};

static float Clamp(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static float Deadzone(SHORT value, SHORT deadzone)
{
    if (value > -deadzone && value < deadzone) return 0.0f;
    return Clamp((float)value / 32767.0f, -1.0f, 1.0f);
}

static void ResetPlayer(int index)
{
    g_players[index].camera.position = Vec3((index - 1.5f) * 1.2f, EYE_HEIGHT, -8.0f);
    g_players[index].camera.yaw = 0.0f;
    g_players[index].camera.pitch = 0.0f;
    g_players[index].verticalVelocity = 0.0f;
    g_players[index].selectedSlot = 0;
    g_players[index].grounded = true;
}

static void AddBlock(const Vec3& position, float halfSize, DWORD color)
{
    if (g_blockCount >= MAX_BLOCKS) return;
    g_blocks[g_blockCount].position = position;
    g_blocks[g_blockCount].halfSize = halfSize;
    g_blocks[g_blockCount].color = color;
    ++g_blockCount;
}

static void InitializeWorld()
{
    g_blockCount = 0;
    // A low obstacle and a small step make the collision test immediately visible.
    AddBlock(Vec3(0, 0.5f, 4), 0.5f, g_itemColors[0]);
    AddBlock(Vec3(2.5f, 0.5f, 5), 0.5f, g_itemColors[1]);
    AddBlock(Vec3(-2.0f, 1.0f, 5), 1.0f, g_itemColors[2]);
}

static void UpdateController(int index)
{
    Controller& pad = g_pads[index];
    pad.previous = pad.current;

    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(state));
    DWORD result = XInputGetState(index, &state);
    pad.connected = (result == ERROR_SUCCESS);
    if (pad.connected) pad.current = state.Gamepad;
    else ZeroMemory(&pad.current, sizeof(pad.current));

    pad.lx = Deadzone(pad.current.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    pad.ly = Deadzone(pad.current.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    pad.rx = Deadzone(pad.current.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
    pad.ry = Deadzone(pad.current.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
    pad.leftTrigger = (float)pad.current.bLeftTrigger / 255.0f;
    pad.rightTrigger = (float)pad.current.bRightTrigger / 255.0f;

    WORD now = pad.current.wButtons;
    WORD old = pad.previous.wButtons;
    pad.pressedA = (now & XINPUT_GAMEPAD_A) && !(old & XINPUT_GAMEPAD_A);
    pad.pressedB = (now & XINPUT_GAMEPAD_B) && !(old & XINPUT_GAMEPAD_B);
    pad.pressedX = (now & XINPUT_GAMEPAD_X) && !(old & XINPUT_GAMEPAD_X);
    pad.pressedY = (now & XINPUT_GAMEPAD_Y) && !(old & XINPUT_GAMEPAD_Y);
    pad.pressedBack = (now & XINPUT_GAMEPAD_BACK) && !(old & XINPUT_GAMEPAD_BACK);
    pad.pressedStart = (now & XINPUT_GAMEPAD_START) && !(old & XINPUT_GAMEPAD_START);
    pad.pressedDpadLeft = (now & XINPUT_GAMEPAD_DPAD_LEFT) && !(old & XINPUT_GAMEPAD_DPAD_LEFT);
    pad.pressedDpadRight = (now & XINPUT_GAMEPAD_DPAD_RIGHT) && !(old & XINPUT_GAMEPAD_DPAD_RIGHT);
}

// Axis-aligned character-vs-block collision. The player is an upright box/capsule
// approximation; movement is resolved one axis at a time to slide along walls.
static bool PlayerOverlapsBlock(float x, float feetY, float z, const Block& b)
{
    float minX = x - PLAYER_RADIUS;
    float maxX = x + PLAYER_RADIUS;
    float minY = feetY;
    float maxY = feetY + PLAYER_HEIGHT;
    float minZ = z - PLAYER_RADIUS;
    float maxZ = z + PLAYER_RADIUS;

    float bMinX = b.position.x - b.halfSize;
    float bMaxX = b.position.x + b.halfSize;
    float bMinY = b.position.y - b.halfSize;
    float bMaxY = b.position.y + b.halfSize;
    float bMinZ = b.position.z - b.halfSize;
    float bMaxZ = b.position.z + b.halfSize;

    return (maxX > bMinX && minX < bMaxX &&
            maxY > bMinY && minY < bMaxY &&
            maxZ > bMinZ && minZ < bMaxZ);
}

static bool CollidesAt(float x, float feetY, float z)
{
    if (feetY < -0.001f) return true; // Flat world floor.
    for (int i = 0; i < g_blockCount; ++i)
        if (PlayerOverlapsBlock(x, feetY, z, g_blocks[i]))
            return true;
    return false;
}

static void AddLine(const Vec3& a, const Vec3& b, DWORD color)
{
    if (g_lineCount + 2 > MAX_LINE_VERTICES) return;
    g_lines[g_lineCount].x=a.x; g_lines[g_lineCount].y=a.y; g_lines[g_lineCount].z=a.z;
    g_lines[g_lineCount++].color=color;
    g_lines[g_lineCount].x=b.x; g_lines[g_lineCount].y=b.y; g_lines[g_lineCount].z=b.z;
    g_lines[g_lineCount++].color=color;
}

static void AddTriangle(const Vec3& a, const Vec3& b, const Vec3& c, DWORD color)
{
    if (g_triangleCount + 3 > MAX_TRI_VERTICES) return;
    g_triangles[g_triangleCount].x=a.x; g_triangles[g_triangleCount].y=a.y; g_triangles[g_triangleCount].z=a.z;
    g_triangles[g_triangleCount++].color=color;
    g_triangles[g_triangleCount].x=b.x; g_triangles[g_triangleCount].y=b.y; g_triangles[g_triangleCount].z=b.z;
    g_triangles[g_triangleCount++].color=color;
    g_triangles[g_triangleCount].x=c.x; g_triangles[g_triangleCount].y=c.y; g_triangles[g_triangleCount].z=c.z;
    g_triangles[g_triangleCount++].color=color;
}

// Triangle filler: each box face is submitted as two filled triangles.
static void AddQuad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, DWORD color)
{
    AddTriangle(a, b, c, color);
    AddTriangle(a, c, d, color);
}

static void AddSolidBox(const Vec3& center, const Vec3& half, DWORD color)
{
    Vec3 v[8];
    v[0]=Vec3(center.x-half.x,center.y-half.y,center.z-half.z);
    v[1]=Vec3(center.x+half.x,center.y-half.y,center.z-half.z);
    v[2]=Vec3(center.x+half.x,center.y+half.y,center.z-half.z);
    v[3]=Vec3(center.x-half.x,center.y+half.y,center.z-half.z);
    v[4]=Vec3(center.x-half.x,center.y-half.y,center.z+half.z);
    v[5]=Vec3(center.x+half.x,center.y-half.y,center.z+half.z);
    v[6]=Vec3(center.x+half.x,center.y+half.y,center.z+half.z);
    v[7]=Vec3(center.x-half.x,center.y+half.y,center.z+half.z);

    AddQuad(v[0],v[1],v[2],v[3],color);
    AddQuad(v[5],v[4],v[7],v[6],color);
    AddQuad(v[4],v[0],v[3],v[7],color);
    AddQuad(v[1],v[5],v[6],v[2],color);
    AddQuad(v[3],v[2],v[6],v[7],color);
    AddQuad(v[4],v[5],v[1],v[0],color);

    DWORD edge = D3DCOLOR_ARGB(255, 22, 28, 35);
    static const int edges[12][2] =
    {
        {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},
        {6,7},{7,4},{0,4},{1,5},{2,6},{3,7}
    };
    for (int i=0;i<12;++i) AddLine(v[edges[i][0]],v[edges[i][1]],edge);
}

static void BuildScene(float renderDistance, const Vec3& cameraPosition, int detail)
{
    g_triangleCount = 0;
    g_lineCount = 0;

    int gridRadius = (g_playerCount == 1) ? 12 : ((g_playerCount == 2) ? 8 : 5);
    if (gridRadius < 3) gridRadius = 3;
    if (detail < 1) detail = 1;

    if (g_showGrid)
    {
        DWORD gridColor = D3DCOLOR_ARGB(255, 45, 57, 70);
        for (int i=-gridRadius;i<=gridRadius;i+=detail)
        {
            AddLine(Vec3((float)i,0,(float)-gridRadius),Vec3((float)i,0,(float)gridRadius),gridColor);
            AddLine(Vec3((float)-gridRadius,0,(float)i),Vec3((float)gridRadius,0,(float)i),gridColor);
        }
    }

    AddLine(Vec3(0,0.01f,0),Vec3(3,0.01f,0),D3DCOLOR_ARGB(255,230,70,70));
    AddLine(Vec3(0,0.01f,0),Vec3(0,3,0),D3DCOLOR_ARGB(255,80,220,100));
    AddLine(Vec3(0,0.01f,0),Vec3(0,0.01f,3),D3DCOLOR_ARGB(255,80,150,255));

    for (int i=0;i<g_blockCount;++i)
    {
        const Block& b = g_blocks[i];
        float dx=b.position.x-cameraPosition.x;
        float dy=b.position.y-cameraPosition.y;
        float dz=b.position.z-cameraPosition.z;
        if (dx*dx+dy*dy+dz*dz <= renderDistance*renderDistance)
            AddSolidBox(b.position,Vec3(b.halfSize,b.halfSize,b.halfSize),b.color);
    }
}

static D3DMATRIX MatrixIdentity()
{
    D3DMATRIX m;
    ZeroMemory(&m,sizeof(m));
    m._11=m._22=m._33=m._44=1.0f;
    return m;
}

static D3DMATRIX MatrixLookAtLH(const Vec3& eye,const Vec3& at,const Vec3& up)
{
    Vec3 z(at.x-eye.x,at.y-eye.y,at.z-eye.z);
    float len=sqrtf(z.x*z.x+z.y*z.y+z.z*z.z);
    if(len<0.0001f) len=1.0f;
    z.x/=len; z.y/=len; z.z/=len;
    Vec3 x(up.y*z.z-up.z*z.y,up.z*z.x-up.x*z.z,up.x*z.y-up.y*z.x);
    len=sqrtf(x.x*x.x+x.y*x.y+x.z*x.z);
    if(len<0.0001f) len=1.0f;
    x.x/=len; x.y/=len; x.z/=len;
    Vec3 y(z.y*x.z-z.z*x.y,z.z*x.x-z.x*x.z,z.x*x.y-z.y*x.x);

    D3DMATRIX m;
    m._11=x.x; m._12=y.x; m._13=z.x; m._14=0;
    m._21=x.y; m._22=y.y; m._23=z.y; m._24=0;
    m._31=x.z; m._32=y.z; m._33=z.z; m._34=0;
    m._41=-(x.x*eye.x+x.y*eye.y+x.z*eye.z);
    m._42=-(y.x*eye.x+y.y*eye.y+y.z*eye.z);
    m._43=-(z.x*eye.x+z.y*eye.y+z.z*eye.z);
    m._44=1;
    return m;
}

static D3DMATRIX MatrixPerspectiveFovLH(float fov,float aspect,float zn,float zf)
{
    D3DMATRIX m;
    ZeroMemory(&m,sizeof(m));
    float ys=1.0f/tanf(fov*0.5f);
    m._11=ys/aspect;
    m._22=ys;
    m._33=zf/(zf-zn);
    m._34=1.0f;
    m._43=-zn*zf/(zf-zn);
    return m;
}

static bool InitializeGraphics()
{
    g_d3d=Direct3DCreate9(D3D_SDK_VERSION);
    if(!g_d3d) return false;

    D3DPRESENT_PARAMETERS pp;
    ZeroMemory(&pp,sizeof(pp));
    pp.BackBufferWidth=SCREEN_W;
    pp.BackBufferHeight=SCREEN_H;
    pp.BackBufferFormat=D3DFMT_X8R8G8B8;
    pp.BackBufferCount=2;
    pp.MultiSampleType=D3DMULTISAMPLE_NONE;
    pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow=0;
    pp.Windowed=FALSE;
    pp.EnableAutoDepthStencil=TRUE;
    pp.AutoDepthStencilFormat=D3DFMT_D24S8;
    pp.FullScreen_RefreshRateInHz=60;
    pp.PresentationInterval=D3DPRESENT_INTERVAL_ONE;

    HRESULT hr=g_d3d->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,0,
        D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,&g_device);
    if(FAILED(hr))
    {
        g_d3d->Release(); g_d3d=0;
        return false;
    }

    g_device->SetRenderState(D3DRS_LIGHTING,FALSE);
    g_device->SetRenderState(D3DRS_ZENABLE,TRUE);
    g_device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
    g_device->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
    g_device->SetRenderState(D3DRS_SHADEMODE,D3DSHADE_GOURAUD);
    g_device->SetFVF(BW_FVF);
    return true;
}

static void PlaceBlockInFront(int index)
{
    Player& p=g_players[index];
    Vec3 forward(sinf(p.camera.yaw),0,cosf(p.camera.yaw));
    Vec3 place(p.camera.position.x+forward.x*3.0f,
               0.5f,
               p.camera.position.z+forward.z*3.0f);
    place.x=floorf(place.x+0.5f);
    place.z=floorf(place.z+0.5f);

    // Do not place a block intersecting the placing player's body.
    float feet=p.camera.position.y-EYE_HEIGHT;
    Block candidate;
    candidate.position=place;
    candidate.halfSize=0.5f;
    if(!PlayerOverlapsBlock(p.camera.position.x,feet,p.camera.position.z,candidate))
        AddBlock(place,0.5f,g_itemColors[p.selectedSlot]);
}

static void UpdatePlayer(int index,float dt)
{
    Controller& pad=g_pads[index];
    Player& p=g_players[index];
    Camera& camera=p.camera;

    const float moveSpeed=g_editorMode?8.0f:5.0f;
    const float lookSpeed=1.8f;
    camera.yaw+=pad.rx*lookSpeed*dt;
    camera.pitch=Clamp(camera.pitch-pad.ry*lookSpeed*dt,-1.45f,1.45f);

    float fx=sinf(camera.yaw), fz=cosf(camera.yaw);
    float rx=cosf(camera.yaw), rz=-sinf(camera.yaw);
    float moveX=(fx*pad.ly+rx*pad.lx)*moveSpeed*dt;
    float moveZ=(fz*pad.ly+rz*pad.lx)*moveSpeed*dt;

    if(g_editorMode)
    {
        camera.position.x+=moveX;
        camera.position.z+=moveZ;
        camera.position.y+=(pad.rightTrigger-pad.leftTrigger)*moveSpeed*dt;
        if(pad.pressedA)
        {
            Vec3 place(camera.position.x+fx*4.0f,camera.position.y-0.5f,camera.position.z+fz*4.0f);
            place.x=floorf(place.x+0.5f); place.y=floorf(place.y+0.5f); place.z=floorf(place.z+0.5f);
            AddBlock(place,0.5f,g_itemColors[p.selectedSlot]);
        }
        if(pad.pressedX && g_blockCount>0) --g_blockCount;
        if(pad.pressedB) g_showGrid=!g_showGrid;
    }
    else
    {
        float feet=camera.position.y-EYE_HEIGHT;
        float nextX=camera.position.x+moveX;
        float nextZ=camera.position.z+moveZ;

        // Resolve horizontal movement independently so the character can slide along walls.
        if(!CollidesAt(nextX,feet,camera.position.z))
            camera.position.x=nextX;
        if(!CollidesAt(camera.position.x,feet,nextZ))
            camera.position.z=nextZ;

        if(pad.pressedA && p.grounded)
        {
            p.verticalVelocity=6.0f;
            p.grounded=false;
        }

        p.verticalVelocity-=GRAVITY*dt;
        float nextFeet=feet+p.verticalVelocity*dt;
        bool landed=false;
        if(p.verticalVelocity<=0.0f)
        {
            float landingHeight=-100000.0f;
            if(feet>=0.0f && nextFeet<=0.0f)
                landingHeight=0.0f;

            // Detect crossing a block top during this frame, not just exact contact.
            for(int b=0;b<g_blockCount;++b)
            {
                const Block& block=g_blocks[b];
                float top=block.position.y+block.halfSize;
                bool overlapsXZ=
                    camera.position.x+PLAYER_RADIUS>block.position.x-block.halfSize &&
                    camera.position.x-PLAYER_RADIUS<block.position.x+block.halfSize &&
                    camera.position.z+PLAYER_RADIUS>block.position.z-block.halfSize &&
                    camera.position.z-PLAYER_RADIUS<block.position.z+block.halfSize;
                if(overlapsXZ && feet>=top-0.02f && nextFeet<=top && top>landingHeight)
                    landingHeight=top;
            }

            if(landingHeight>-99999.0f)
            {
                nextFeet=landingHeight;
                p.verticalVelocity=0.0f;
                p.grounded=true;
                landed=true;
            }
        }

        if(!landed)
        {
            p.grounded=false;
            // Head collision stops upward movement against block undersides.
            if(p.verticalVelocity>0.0f && CollidesAt(camera.position.x,nextFeet,camera.position.z))
            {
                nextFeet=feet;
                p.verticalVelocity=0.0f;
            }
        }
        camera.position.y=nextFeet+EYE_HEIGHT;

        if(pad.pressedDpadLeft)
            p.selectedSlot=(p.selectedSlot+INVENTORY_SLOTS-1)%INVENTORY_SLOTS;
        if(pad.pressedDpadRight)
            p.selectedSlot=(p.selectedSlot+1)%INVENTORY_SLOTS;
        if(pad.pressedX) PlaceBlockInFront(index);
    }

    if(pad.pressedStart)
    {
        camera.position=Vec3((index-1.5f)*1.2f,g_editorMode?2.5f:EYE_HEIGHT,-8.0f);
        camera.yaw=0.0f;
        camera.pitch=0.0f;
        p.verticalVelocity=0.0f;
        p.grounded=true;
    }
}

static Viewport GetViewport(int slot,int count)
{
    Viewport v;
    if(count<=1) { v.x=0; v.y=0; v.width=SCREEN_W; v.height=SCREEN_H; }
    else if(count==2) { v.x=slot*(SCREEN_W/2); v.y=0; v.width=SCREEN_W/2; v.height=SCREEN_H; }
    else { v.x=(slot%2)*(SCREEN_W/2); v.y=(slot/2)*(SCREEN_H/2); v.width=SCREEN_W/2; v.height=SCREEN_H/2; }
    return v;
}

static void AddHudLine(HudVertex* v,int& count,float x1,float y1,float x2,float y2,DWORD color)
{
    if(count+2>256) return;
    v[count].x=x1; v[count].y=y1; v[count].z=0; v[count].rhw=1; v[count++].color=color;
    v[count].x=x2; v[count].y=y2; v[count].z=0; v[count].rhw=1; v[count++].color=color;
}

static void AddHudRect(HudVertex* v,int& count,float x,float y,float w,float h,DWORD color)
{
    AddHudLine(v,count,x,y,x+w,y,color);
    AddHudLine(v,count,x+w,y,x+w,y+h,color);
    AddHudLine(v,count,x+w,y+h,x,y+h,color);
    AddHudLine(v,count,x,y+h,x,y,color);
}

static void RenderHud(int playerIndex,const Viewport& viewport)
{
    HudVertex hud[256];
    int count=0;
    Player& p=g_players[playerIndex];
    float scale=(g_playerCount==1)?1.0f:0.72f;
    float slotW=34.0f*scale, slotH=30.0f*scale, gap=4.0f*scale;
    float total=INVENTORY_SLOTS*slotW+(INVENTORY_SLOTS-1)*gap;
    float start=viewport.x+(viewport.width-total)*0.5f;
    float y=viewport.y+viewport.height-slotH-18.0f*scale;
    DWORD modeColor=g_editorMode?D3DCOLOR_ARGB(255,235,165,65):D3DCOLOR_ARGB(255,90,190,255);

    for(int i=0;i<INVENTORY_SLOTS;++i)
    {
        float x=start+i*(slotW+gap);
        AddHudRect(hud,count,x,y,slotW,slotH,
            i==p.selectedSlot?modeColor:D3DCOLOR_ARGB(255,125,135,150));
        AddHudRect(hud,count,x+9*scale,y+8*scale,16*scale,14*scale,g_itemColors[i]);
    }
    AddHudRect(hud,count,viewport.x+12,viewport.y+12,76,8,modeColor);
    AddHudRect(hud,count,viewport.x+viewport.width-22,viewport.y+12,10,10,
        D3DCOLOR_ARGB(255,80+playerIndex*35,150,235));

    D3DVIEWPORT9 full;
    full.X=0; full.Y=0; full.Width=SCREEN_W; full.Height=SCREEN_H;
    full.MinZ=0.0f; full.MaxZ=1.0f;
    g_device->SetViewport(&full);
    g_device->SetVertexShader(0);
    g_device->SetPixelShader(0);
    g_device->SetFVF(BW_HUD_FVF);
    if(count>0) g_device->DrawPrimitiveUP(D3DPT_LINELIST,count/2,hud,sizeof(HudVertex));
}

static void RenderCharacter(int playerIndex,float renderDistance)
{
    const Player& p=g_players[playerIndex];
    Vec3 feet(p.camera.position.x,p.camera.position.y-EYE_HEIGHT,p.camera.position.z);
    // Each local character is visible to the other split-screen cameras.
    AddSolidBox(Vec3(feet.x,feet.y+PLAYER_HEIGHT*0.5f,feet.z),
                Vec3(PLAYER_RADIUS,PLAYER_HEIGHT*0.5f,PLAYER_RADIUS),
                D3DCOLOR_ARGB(255,75+playerIndex*35,145,235));
    AddSolidBox(Vec3(feet.x,feet.y+PLAYER_HEIGHT+0.12f,feet.z),
                Vec3(0.23f,0.23f,0.23f),
                D3DCOLOR_ARGB(255,220,185,145));
    (void)renderDistance;
}

static void RenderPlayerView(int playerIndex,const Viewport& viewport,float renderDistance,int detail)
{
    Player& p=g_players[playerIndex];
    Camera& camera=p.camera;
    BuildScene(renderDistance,camera.position,detail);

    // Draw all player avatars, then the world. Avatars are simple filled boxes for now.
    for(int i=0;i<g_playerCount;++i)
        RenderCharacter(g_activePlayers[i],renderDistance);

    Vec3 forward(sinf(camera.yaw)*cosf(camera.pitch),sinf(camera.pitch),cosf(camera.yaw)*cosf(camera.pitch));
    Vec3 target(camera.position.x+forward.x,camera.position.y+forward.y,camera.position.z+forward.z);

    D3DVIEWPORT9 vp;
    vp.X=viewport.x; vp.Y=viewport.y; vp.Width=viewport.width; vp.Height=viewport.height;
    vp.MinZ=0.0f; vp.MaxZ=1.0f;
    g_device->SetViewport(&vp);

    D3DMATRIX world=MatrixIdentity();
    D3DMATRIX view=MatrixLookAtLH(camera.position,target,Vec3(0,1,0));
    D3DMATRIX projection=MatrixPerspectiveFovLH(PI_F/3.0f,
        (float)viewport.width/(float)viewport.height,0.1f,renderDistance);
    g_device->SetTransform(D3DTS_WORLD,&world);
    g_device->SetTransform(D3DTS_VIEW,&view);
    g_device->SetTransform(D3DTS_PROJECTION,&projection);
    g_device->SetVertexShader(0);
    g_device->SetPixelShader(0);
    g_device->SetFVF(BW_FVF);

    if(g_triangleCount>0)
        g_device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,g_triangleCount/3,g_triangles,sizeof(Vertex));
    if(g_lineCount>0)
        g_device->DrawPrimitiveUP(D3DPT_LINELIST,g_lineCount/2,g_lines,sizeof(Vertex));

    RenderHud(playerIndex,viewport);
}

static void UpdateAdaptiveQuality(float frameSeconds)
{
    if(frameSeconds<=0.0f) return;
    float instantFps=1.0f/frameSeconds;
    g_fpsAverage=g_fpsAverage*0.90f+instantFps*0.10f;

    // Hysteresis avoids rapidly changing detail around the threshold.
    if(g_fpsAverage<48.0f && g_qualityScale>0.55f)
        g_qualityScale-=0.05f;
    else if(g_fpsAverage>57.0f && g_qualityScale<1.0f)
        g_qualityScale+=0.025f;
    g_qualityScale=Clamp(g_qualityScale,0.55f,1.0f);
}

static void RenderFrame()
{
    if(!g_device) return;

    float playerScale=(g_playerCount==1)?1.0f:((g_playerCount==2)?0.72f:0.48f);
    float renderDistance=120.0f*playerScale*g_qualityScale;
    int detail=(g_qualityScale>0.85f)?1:((g_qualityScale>0.70f)?2:3);

    g_device->Clear(0,0,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,
        D3DCOLOR_ARGB(255,16,22,32),1.0f,0);
    g_device->BeginScene();

    for(int i=0;i<g_playerCount;++i)
    {
        Viewport viewport=GetViewport(i,g_playerCount);
        RenderPlayerView(g_activePlayers[i],viewport,renderDistance,detail);
    }

    g_device->EndScene();
    g_device->Present(0,0,0,0);
}

static void Shutdown()
{
    if(g_device) { g_device->Release(); g_device=0; }
    if(g_d3d) { g_d3d->Release(); g_d3d=0; }
}

int main()
{
    ZeroMemory(g_pads,sizeof(g_pads));
    for(int i=0;i<PLAYER_LIMIT;++i) ResetPlayer(i);
    InitializeWorld();

    if(!InitializeGraphics()) return 1;
    g_lastTick=GetTickCount();

    for(;;)
    {
        DWORD now=GetTickCount();
        DWORD elapsed=now-g_lastTick;
        if(elapsed<FRAME_CAP_MS)
        {
            Sleep(1);
            continue;
        }

        float dt=(float)elapsed/1000.0f;
        g_lastTick=now;
        dt=Clamp(dt,0.001f,0.05f);
        UpdateAdaptiveQuality(dt);

        int connected=0;
        for(int i=0;i<PLAYER_LIMIT;++i)
        {
            UpdateController(i);
            if(g_pads[i].connected)
                g_activePlayers[connected++]=i;
        }
        if(connected==0) g_activePlayers[0]=0;
        g_playerCount=connected>0?connected:1;

        for(int i=0;i<PLAYER_LIMIT;++i)
        {
            if(g_pads[i].connected && g_pads[i].pressedBack)
            {
                g_editorMode=!g_editorMode;
                break;
            }
        }

        for(int i=0;i<PLAYER_LIMIT;++i)
            if(g_pads[i].connected) UpdatePlayer(i,dt);
        if(connected==0) UpdatePlayer(0,dt);

        RenderFrame();
    }

    Shutdown();
    return 0;
}
