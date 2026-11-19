#include <xtl.h>
#include <d3d9.h>
#include <xinput.h>
#include <math.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "xinput.lib")

// BlockWorks 360 - controller, split-screen, and rendering foundation.
// This is deliberately a small immediate-mode renderer, not the final engine.

static const int SCREEN_W = 1280;
static const int SCREEN_H = 720;
static const int MAX_VERTICES = 8192;
static const int MAX_BLOCKS = 128;
static const int PLAYER_LIMIT = 4;
static const int INVENTORY_SLOTS = 8;
static const float PI_F = 3.14159265358979323846f;

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
static Vertex g_vertices[MAX_VERTICES];
static int g_vertexCount = 0;

struct Camera
{
    Vec3 position;
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
    float v = (float)value / 32767.0f;
    return Clamp(v, -1.0f, 1.0f);
}

static void ResetPlayer(int index)
{
    g_players[index].camera.position = Vec3((index - 1.5f) * 1.2f, 1.7f, -8.0f);
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
    AddBlock(Vec3(0, 1, 4), 1.0f, g_itemColors[0]);
    AddBlock(Vec3(2.5f, 0.5f, 5), 0.5f, g_itemColors[1]);
}

static void UpdateController(int index)
{
    Controller& pad = g_pads[index];
    pad.previous = pad.current;

    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(state));
    DWORD result = XInputGetState(index, &state);
    pad.connected = (result == ERROR_SUCCESS);

    if (pad.connected)
        pad.current = state.Gamepad;
    else
        ZeroMemory(&pad.current, sizeof(pad.current));

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

static void AddLine(const Vec3& a, const Vec3& b, DWORD color)
{
    if (g_vertexCount + 2 > MAX_VERTICES) return;
    g_vertices[g_vertexCount].x = a.x;
    g_vertices[g_vertexCount].y = a.y;
    g_vertices[g_vertexCount].z = a.z;
    g_vertices[g_vertexCount++].color = color;
    g_vertices[g_vertexCount].x = b.x;
    g_vertices[g_vertexCount].y = b.y;
    g_vertices[g_vertexCount].z = b.z;
    g_vertices[g_vertexCount++].color = color;
}

static void AddCube(const Vec3& center, float halfSize, DWORD color)
{
    Vec3 v[8];
    const float sx[8] = {-1, 1, 1,-1,-1, 1, 1,-1};
    const float sy[8] = {-1,-1, 1, 1,-1,-1, 1, 1};
    const float sz[8] = {-1,-1,-1,-1, 1, 1, 1, 1};

    for (int i = 0; i < 8; ++i)
        v[i] = Vec3(center.x + sx[i]*halfSize,
                    center.y + sy[i]*halfSize,
                    center.z + sz[i]*halfSize);

    static const int edges[12][2] =
    {
        {0,1},{1,2},{2,3},{3,0},
        {4,5},{5,6},{6,7},{7,4},
        {0,4},{1,5},{2,6},{3,7}
    };

    for (int e = 0; e < 12; ++e)
        AddLine(v[edges[e][0]], v[edges[e][1]], color);
}

static void BuildScene(float renderDistance, const Vec3& cameraPosition)
{
    g_vertexCount = 0;
    int gridRadius = (g_playerCount == 1) ? 12 : ((g_playerCount == 2) ? 8 : 5);

    if (g_showGrid)
    {
        const DWORD gridColor = D3DCOLOR_ARGB(255, 45, 57, 70);
        for (int i = -gridRadius; i <= gridRadius; ++i)
        {
            AddLine(Vec3((float)i, 0, -gridRadius), Vec3((float)i, 0, gridRadius), gridColor);
            AddLine(Vec3(-gridRadius, 0, (float)i), Vec3(gridRadius, 0, (float)i), gridColor);
        }
    }

    AddLine(Vec3(0, 0.01f, 0), Vec3(3, 0.01f, 0), D3DCOLOR_ARGB(255, 230, 70, 70));
    AddLine(Vec3(0, 0.01f, 0), Vec3(0, 3, 0), D3DCOLOR_ARGB(255, 80, 220, 100));
    AddLine(Vec3(0, 0.01f, 0), Vec3(0, 0.01f, 3), D3DCOLOR_ARGB(255, 80, 150, 255));

    for (int i = 0; i < g_blockCount; ++i)
    {
        const Block& block = g_blocks[i];
        float dx = block.position.x - cameraPosition.x;
        float dy = block.position.y - cameraPosition.y;
        float dz = block.position.z - cameraPosition.z;
        if (dx*dx + dy*dy + dz*dz <= renderDistance*renderDistance)
            AddCube(block.position, block.halfSize, block.color);
    }

}

static D3DMATRIX MatrixIdentity()
{
    D3DMATRIX m;
    ZeroMemory(&m, sizeof(m));
    m._11 = m._22 = m._33 = m._44 = 1.0f;
    return m;
}

static D3DMATRIX MatrixLookAtLH(const Vec3& eye, const Vec3& at, const Vec3& up)
{
    Vec3 z(at.x-eye.x, at.y-eye.y, at.z-eye.z);
    float len = sqrtf(z.x*z.x + z.y*z.y + z.z*z.z);
    if (len < 0.0001f) len = 1.0f;
    z.x /= len; z.y /= len; z.z /= len;

    Vec3 x(up.y*z.z-up.z*z.y, up.z*z.x-up.x*z.z, up.x*z.y-up.y*z.x);
    len = sqrtf(x.x*x.x + x.y*x.y + x.z*x.z);
    if (len < 0.0001f) len = 1.0f;
    x.x /= len; x.y /= len; x.z /= len;

    Vec3 y(z.y*x.z-z.z*x.y, z.z*x.x-z.x*x.z, z.x*x.y-z.y*x.x);

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

static D3DMATRIX MatrixPerspectiveFovLH(float fov, float aspect, float zn, float zf)
{
    D3DMATRIX m;
    ZeroMemory(&m, sizeof(m));
    float yScale = 1.0f / tanf(fov * 0.5f);
    float xScale = yScale / aspect;
    m._11 = xScale;
    m._22 = yScale;
    m._33 = zf / (zf - zn);
    m._34 = 1.0f;
    m._43 = -zn * zf / (zf - zn);
    return m;
}

static bool InitializeGraphics()
{
    g_d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!g_d3d) return false;

    D3DPRESENT_PARAMETERS pp;
    ZeroMemory(&pp, sizeof(pp));
    pp.BackBufferWidth = SCREEN_W;
    pp.BackBufferHeight = SCREEN_H;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 2;
    pp.MultiSampleType = D3DMULTISAMPLE_NONE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow = 0;
    pp.Windowed = FALSE;
    pp.EnableAutoDepthStencil = TRUE;
    pp.AutoDepthStencilFormat = D3DFMT_D24S8;
    pp.FullScreen_RefreshRateInHz = 60;
    pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    HRESULT hr = g_d3d->CreateDevice(
        D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, 0,
        D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &g_device);

    if (FAILED(hr))
    {
        g_d3d->Release();
        g_d3d = 0;
        return false;
    }

    g_device->SetRenderState(D3DRS_LIGHTING, FALSE);
    g_device->SetRenderState(D3DRS_ZENABLE, TRUE);
    g_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    g_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    g_device->SetFVF(BW_FVF);
    return true;
}

static void UpdatePlayer(int index, float dt)
{
    Controller& pad = g_pads[index];
    Player& player = g_players[index];
    Camera& camera = player.camera;

    const float moveSpeed = g_editorMode ? 8.0f : 5.0f;
    const float lookSpeed = 1.8f;

    camera.yaw += pad.rx * lookSpeed * dt;
    camera.pitch -= pad.ry * lookSpeed * dt;
    camera.pitch = Clamp(camera.pitch, -1.45f, 1.45f);

    float forwardX = sinf(camera.yaw);
    float forwardZ = cosf(camera.yaw);
    float rightX = cosf(camera.yaw);
    float rightZ = -sinf(camera.yaw);

    camera.position.x += (forwardX*pad.ly + rightX*pad.lx) * moveSpeed * dt;
    camera.position.z += (forwardZ*pad.ly + rightZ*pad.lx) * moveSpeed * dt;

    if (g_editorMode)
    {
        camera.position.y += (pad.rightTrigger - pad.leftTrigger) * moveSpeed * dt;
        if (pad.pressedA)
        {
            Vec3 forward(sinf(camera.yaw), 0, cosf(camera.yaw));
            Vec3 place(camera.position.x + forward.x*4.0f,
                       camera.position.y - 0.5f,
                       camera.position.z + forward.z*4.0f);
            place.x = floorf(place.x + 0.5f);
            place.y = floorf(place.y + 0.5f);
            place.z = floorf(place.z + 0.5f);
            AddBlock(place, 0.5f, g_itemColors[player.selectedSlot]);
        }
        if (pad.pressedX && g_blockCount > 0)
            --g_blockCount;
        if (pad.pressedB)
            g_showGrid = !g_showGrid;
    }
    else
    {
        if (pad.pressedA && player.grounded)
        {
            player.verticalVelocity = 5.5f;
            player.grounded = false;
        }

        player.verticalVelocity -= 14.0f * dt;
        camera.position.y += player.verticalVelocity * dt;
        if (camera.position.y < 1.7f)
        {
            camera.position.y = 1.7f;
            player.verticalVelocity = 0.0f;
            player.grounded = true;
        }

        if (pad.pressedDpadLeft)
            player.selectedSlot = (player.selectedSlot + INVENTORY_SLOTS - 1) % INVENTORY_SLOTS;
        if (pad.pressedDpadRight)
            player.selectedSlot = (player.selectedSlot + 1) % INVENTORY_SLOTS;

        // X uses the selected inventory item by placing a block in front.
        if (pad.pressedX)
        {
            Vec3 forward(sinf(camera.yaw), 0, cosf(camera.yaw));
            Vec3 place(camera.position.x + forward.x*3.0f,
                       0.5f,
                       camera.position.z + forward.z*3.0f);
            place.x = floorf(place.x + 0.5f);
            place.z = floorf(place.z + 0.5f);
            AddBlock(place, 0.5f, g_itemColors[player.selectedSlot]);
        }
    }

    if (pad.pressedStart)
    {
        // Start recentres this player's camera without changing their inventory.
        camera.position = Vec3((index - 1.5f) * 1.2f, g_editorMode ? 2.5f : 1.7f, -8.0f);
        camera.yaw = 0.0f;
        camera.pitch = 0.0f;
    }
}

static Viewport GetViewport(int playerIndex, int playerCount)
{
    Viewport v;
    if (playerCount <= 1)
    {
        v.x = 0; v.y = 0; v.width = SCREEN_W; v.height = SCREEN_H;
    }
    else if (playerCount == 2)
    {
        v.x = playerIndex * (SCREEN_W / 2);
        v.y = 0; v.width = SCREEN_W / 2; v.height = SCREEN_H;
    }
    else
    {
        v.x = (playerIndex % 2) * (SCREEN_W / 2);
        v.y = (playerIndex / 2) * (SCREEN_H / 2);
        v.width = SCREEN_W / 2; v.height = SCREEN_H / 2;
    }
    return v;
}

static void AddHudLine(HudVertex* verts, int& count,
                       float x1, float y1, float x2, float y2, DWORD color)
{
    if (count + 2 > 256) return;
    verts[count].x=x1; verts[count].y=y1; verts[count].z=0; verts[count].rhw=1; verts[count++].color=color;
    verts[count].x=x2; verts[count].y=y2; verts[count].z=0; verts[count].rhw=1; verts[count++].color=color;
}

static void AddHudRect(HudVertex* verts, int& count, float x, float y,
                       float w, float h, DWORD color)
{
    AddHudLine(verts,count,x,y,x+w,y,color);
    AddHudLine(verts,count,x+w,y,x+w,y+h,color);
    AddHudLine(verts,count,x+w,y+h,x,y+h,color);
    AddHudLine(verts,count,x,y+h,x,y,color);
}

static void RenderHud(int playerIndex, const Viewport& viewport)
{
    HudVertex hud[256];
    int count = 0;
    Player& player = g_players[playerIndex];
    float unit = (g_playerCount == 1) ? 1.0f : 0.72f;
    float slotW = 34.0f * unit;
    float slotH = 30.0f * unit;
    float gap = 4.0f * unit;
    float totalW = INVENTORY_SLOTS * slotW + (INVENTORY_SLOTS - 1) * gap;
    float startX = viewport.x + (viewport.width - totalW) * 0.5f;
    float y = viewport.y + viewport.height - slotH - 18.0f * unit;
    DWORD border = g_editorMode ? D3DCOLOR_ARGB(255, 235, 165, 65) : D3DCOLOR_ARGB(255, 90, 190, 255);

    // The hotbar is a lightweight HUD placeholder; item names/text come later.
    for (int i = 0; i < INVENTORY_SLOTS; ++i)
    {
        float x = startX + i * (slotW + gap);
        DWORD frame = (i == player.selectedSlot) ? border : D3DCOLOR_ARGB(255, 125, 135, 150);
        AddHudRect(hud, count, x, y, slotW, slotH, frame);
        AddHudRect(hud, count, x + 9.0f*unit, y + 8.0f*unit,
                   16.0f*unit, 14.0f*unit, g_itemColors[i]);
    }

    // A mode strip and player-color marker distinguish split-screen views.
    AddHudRect(hud, count, viewport.x + 12, viewport.y + 12, 76, 8,
               g_editorMode ? D3DCOLOR_ARGB(255, 235, 165, 65) : D3DCOLOR_ARGB(255, 90, 190, 255));
    AddHudRect(hud, count, viewport.x + viewport.width - 22, viewport.y + 12,
               10, 10, D3DCOLOR_ARGB(255, 80 + playerIndex*35, 150, 235));

    D3DVIEWPORT9 full;
    full.X = 0; full.Y = 0; full.Width = SCREEN_W; full.Height = SCREEN_H;
    full.MinZ = 0.0f; full.MaxZ = 1.0f;
    g_device->SetViewport(&full);
    g_device->SetVertexShader(0);
    g_device->SetPixelShader(0);
    g_device->SetFVF(BW_HUD_FVF);
    if (count > 0)
        g_device->DrawPrimitiveUP(D3DPT_LINELIST, count/2, hud, sizeof(HudVertex));
}

static void RenderPlayerView(int playerIndex, const Viewport& viewport, float renderDistance)
{
    Player& player = g_players[playerIndex];
    Camera& camera = player.camera;

    BuildScene(renderDistance, camera.position);

    Vec3 forward(sinf(camera.yaw)*cosf(camera.pitch),
                 sinf(camera.pitch),
                 cosf(camera.yaw)*cosf(camera.pitch));
    Vec3 target(camera.position.x+forward.x,
                camera.position.y+forward.y,
                camera.position.z+forward.z);

    D3DVIEWPORT9 d3dViewport;
    d3dViewport.X = viewport.x;
    d3dViewport.Y = viewport.y;
    d3dViewport.Width = viewport.width;
    d3dViewport.Height = viewport.height;
    d3dViewport.MinZ = 0.0f;
    d3dViewport.MaxZ = 1.0f;
    g_device->SetViewport(&d3dViewport);

    D3DMATRIX world = MatrixIdentity();
    D3DMATRIX view = MatrixLookAtLH(camera.position, target, Vec3(0,1,0));
    D3DMATRIX projection = MatrixPerspectiveFovLH(
        PI_F/3.0f, (float)viewport.width/(float)viewport.height, 0.1f, renderDistance);

    g_device->SetTransform(D3DTS_WORLD, &world);
    g_device->SetTransform(D3DTS_VIEW, &view);
    g_device->SetTransform(D3DTS_PROJECTION, &projection);
    g_device->SetVertexShader(0);
    g_device->SetPixelShader(0);
    g_device->SetFVF(BW_FVF);
    if (g_vertexCount > 0)
        g_device->DrawPrimitiveUP(D3DPT_LINELIST, g_vertexCount/2, g_vertices, sizeof(Vertex));

    RenderHud(playerIndex, viewport);
}

static void RenderFrame()
{
    if (!g_device) return;

    float renderDistance = (g_playerCount == 1) ? 120.0f :
                           ((g_playerCount == 2) ? 75.0f : 45.0f);

    g_device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                    D3DCOLOR_ARGB(255, 16, 22, 32), 1.0f, 0);
    g_device->BeginScene();

    for (int i = 0; i < g_playerCount; ++i)
    {
        int playerIndex = g_activePlayers[i];
        Viewport viewport = GetViewport(i, g_playerCount);
        RenderPlayerView(playerIndex, viewport, renderDistance);
    }

    g_device->EndScene();
    g_device->Present(0, 0, 0, 0);
}

static void Shutdown()
{
    if (g_device) { g_device->Release(); g_device = 0; }
    if (g_d3d) { g_d3d->Release(); g_d3d = 0; }
}

int main()
{
    ZeroMemory(g_pads, sizeof(g_pads));
    for (int i = 0; i < PLAYER_LIMIT; ++i)
        ResetPlayer(i);
    InitializeWorld();

    if (!InitializeGraphics())
        return 1;

    g_lastTick = GetTickCount();

    for (;;)
    {
        DWORD now = GetTickCount();
        float dt = (float)(now - g_lastTick) / 1000.0f;
        g_lastTick = now;
        dt = Clamp(dt, 0.001f, 0.05f);

        int connectedCount = 0;
        for (int i = 0; i < PLAYER_LIMIT; ++i)
        {
            UpdateController(i);
            if (g_pads[i].connected)
                g_activePlayers[connectedCount++] = i;
        }

        // Controller 1 remains as a development viewport when no pad is connected.
        if (connectedCount == 0)
            g_activePlayers[0] = 0;
        g_playerCount = connectedCount > 0 ? connectedCount : 1;

        // Back is a global mode toggle. Only the first newly-pressed Back is handled.
        for (int i = 0; i < PLAYER_LIMIT; ++i)
        {
            if (g_pads[i].connected && g_pads[i].pressedBack)
            {
                g_editorMode = !g_editorMode;
                break;
            }
        }

        for (int i = 0; i < PLAYER_LIMIT; ++i)
        {
            if (g_pads[i].connected)
                UpdatePlayer(i, dt);
        }
        if (connectedCount == 0)
            UpdatePlayer(0, dt);

        RenderFrame();
    }

    Shutdown();
    return 0;
}
