#include <xtl.h>
#include <d3d9.h>
#include <xinput.h>
#include <math.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "xinput.lib")

// BlockWorks / Xbox 360 Physics Sandbox
// Milestone 0: controller input + minimal Direct3D 9 renderer.

static const int SCREEN_W = 1280;
static const int SCREEN_H = 720;
static const int MAX_VERTICES = 4096;
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

#define BW_FVF (D3DFVF_XYZ | D3DFVF_DIFFUSE)

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
    bool pressedA;
    bool pressedB;
    bool pressedStart;
};

static Camera g_camera;
static Controller g_pad;
static bool g_showGrid = true;
static DWORD g_lastTick = 0;

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

static void ResetCamera()
{
    g_camera.position = Vec3(0.0f, 2.5f, -8.0f);
    g_camera.yaw = 0.0f;
    g_camera.pitch = -0.12f;
}

static void UpdateController()
{
    g_pad.previous = g_pad.current;

    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(state));
    DWORD result = XInputGetState(0, &state);
    g_pad.connected = (result == ERROR_SUCCESS);

    if (g_pad.connected)
        g_pad.current = state.Gamepad;
    else
        ZeroMemory(&g_pad.current, sizeof(g_pad.current));

    g_pad.lx = Deadzone(g_pad.current.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    g_pad.ly = Deadzone(g_pad.current.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    g_pad.rx = Deadzone(g_pad.current.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
    g_pad.ry = Deadzone(g_pad.current.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);

    WORD now = g_pad.current.wButtons;
    WORD old = g_pad.previous.wButtons;
    g_pad.pressedA = (now & XINPUT_GAMEPAD_A) && !(old & XINPUT_GAMEPAD_A);
    g_pad.pressedB = (now & XINPUT_GAMEPAD_B) && !(old & XINPUT_GAMEPAD_B);
    g_pad.pressedStart = (now & XINPUT_GAMEPAD_START) && !(old & XINPUT_GAMEPAD_START);
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

static void BuildScene()
{
    g_vertexCount = 0;

    if (g_showGrid)
    {
        const DWORD gridColor = D3DCOLOR_ARGB(255, 45, 57, 70);
        for (int i = -12; i <= 12; ++i)
        {
            AddLine(Vec3((float)i, 0, -12), Vec3((float)i, 0, 24), gridColor);
            AddLine(Vec3(-12, 0, (float)i), Vec3(12, 0, (float)i), gridColor);
        }
    }

    AddLine(Vec3(0, 0.01f, 0), Vec3(3, 0.01f, 0), D3DCOLOR_ARGB(255, 230, 70, 70));
    AddLine(Vec3(0, 0.01f, 0), Vec3(0, 3, 0), D3DCOLOR_ARGB(255, 80, 220, 100));
    AddLine(Vec3(0, 0.01f, 0), Vec3(0, 0.01f, 3), D3DCOLOR_ARGB(255, 80, 150, 255));

    AddCube(Vec3(0, 1, 4), 1.0f, D3DCOLOR_ARGB(255, 90, 205, 155));
    AddCube(Vec3(2.5f, 0.5f, 5), 0.5f, D3DCOLOR_ARGB(255, 90, 160, 235));
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
    Vec3 z = Vec3(at.x-eye.x, at.y-eye.y, at.z-eye.z);
    float len = sqrtf(z.x*z.x + z.y*z.y + z.z*z.z);
    if (len < 0.0001f) len = 1.0f;
    z.x /= len; z.y /= len; z.z /= len;

    Vec3 x = Vec3(up.y*z.z-up.z*z.y, up.z*z.x-up.x*z.z, up.x*z.y-up.y*z.x);
    len = sqrtf(x.x*x.x + x.y*x.y + x.z*x.z);
    if (len < 0.0001f) len = 1.0f;
    x.x /= len; x.y /= len; x.z /= len;

    Vec3 y = Vec3(z.y*x.z-z.z*x.y, z.z*x.x-z.x*x.z, z.x*x.y-z.y*x.x);

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
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        0,
        D3DCREATE_HARDWARE_VERTEXPROCESSING,
        &pp,
        &g_device);

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

static void UpdateCamera(float dt)
{
    const float moveSpeed = 7.0f;
    const float lookSpeed = 1.8f;

    g_camera.yaw += g_pad.rx * lookSpeed * dt;
    g_camera.pitch -= g_pad.ry * lookSpeed * dt;
    g_camera.pitch = Clamp(g_camera.pitch, -1.45f, 1.45f);

    float forwardX = sinf(g_camera.yaw);
    float forwardZ = cosf(g_camera.yaw);
    float rightX = cosf(g_camera.yaw);
    float rightZ = -sinf(g_camera.yaw);

    g_camera.position.x += (forwardX*g_pad.ly + rightX*g_pad.lx) * moveSpeed * dt;
    g_camera.position.z += (forwardZ*g_pad.ly + rightZ*g_pad.lx) * moveSpeed * dt;
}

static void RenderFrame()
{
    if (!g_device) return;

    BuildScene();

    Vec3 forward(
        sinf(g_camera.yaw) * cosf(g_camera.pitch),
        sinf(g_camera.pitch),
        cosf(g_camera.yaw) * cosf(g_camera.pitch));

    Vec3 target(
        g_camera.position.x + forward.x,
        g_camera.position.y + forward.y,
        g_camera.position.z + forward.z);

    D3DMATRIX world = MatrixIdentity();
    D3DMATRIX view = MatrixLookAtLH(g_camera.position, target, Vec3(0,1,0));
    D3DMATRIX projection = MatrixPerspectiveFovLH(PI_F/3.0f, (float)SCREEN_W/(float)SCREEN_H, 0.1f, 250.0f);

    g_device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                    D3DCOLOR_ARGB(255, 16, 22, 32), 1.0f, 0);
    g_device->BeginScene();

    g_device->SetTransform(D3DTS_WORLD, &world);
    g_device->SetTransform(D3DTS_VIEW, &view);
    g_device->SetTransform(D3DTS_PROJECTION, &projection);
    g_device->SetVertexShader(0);
    g_device->SetPixelShader(0);
    g_device->SetFVF(BW_FVF);

    if (g_vertexCount > 0)
        g_device->DrawPrimitiveUP(D3DPT_LINELIST, g_vertexCount/2, g_vertices, sizeof(Vertex));

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
    ZeroMemory(&g_pad, sizeof(g_pad));
    ResetCamera();

    if (!InitializeGraphics())
        return 1;

    g_lastTick = GetTickCount();

    for (;;)
    {
        DWORD now = GetTickCount();
        float dt = (float)(now - g_lastTick) / 1000.0f;
        g_lastTick = now;
        dt = Clamp(dt, 0.001f, 0.05f);

        UpdateController();

        if (g_pad.pressedA || g_pad.pressedStart)
            ResetCamera();

        if (g_pad.pressedB)
            g_showGrid = !g_showGrid;

        UpdateCamera(dt);
        RenderFrame();
    }

    Shutdown();
    return 0;
}
