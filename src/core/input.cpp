#include "game/game.h"
static float axis_value(SHORT v,SHORT deadzone)
{
    if(v>-deadzone && v<deadzone) return 0.0f;
    return bw_clamp((float)v/32767.0f,-1.0f,1.0f);
}
void bw_input_poll(BwGame *game)
{
    int count=0;
    for(int i=0;i<BW_MAX_PLAYERS;++i) {
        BwPad *p=&game->pads[i]; p->previous=p->current;
        XINPUT_STATE state; ZeroMemory(&state,sizeof(state));
        p->connected=XInputGetState((DWORD)i,&state)==ERROR_SUCCESS;
        if(p->connected) p->current=state.Gamepad; else ZeroMemory(&p->current,sizeof(p->current));
        WORD now=p->current.wButtons,old=p->previous.wButtons;
#define EDGE(field,button) p->field=((now&(button))!=0)&&((old&(button))==0)
        EDGE(pressed_a,XINPUT_GAMEPAD_A); EDGE(pressed_b,XINPUT_GAMEPAD_B);
        EDGE(pressed_x,XINPUT_GAMEPAD_X); EDGE(pressed_y,XINPUT_GAMEPAD_Y);
        EDGE(pressed_back,XINPUT_GAMEPAD_BACK); EDGE(pressed_start,XINPUT_GAMEPAD_START);
        EDGE(pressed_left,XINPUT_GAMEPAD_DPAD_LEFT); EDGE(pressed_right,XINPUT_GAMEPAD_DPAD_RIGHT);
        EDGE(pressed_up,XINPUT_GAMEPAD_DPAD_UP); EDGE(pressed_down,XINPUT_GAMEPAD_DPAD_DOWN);
        EDGE(pressed_lthumb,XINPUT_GAMEPAD_LEFT_THUMB); EDGE(pressed_lb,XINPUT_GAMEPAD_LEFT_SHOULDER); EDGE(pressed_rb,XINPUT_GAMEPAD_RIGHT_SHOULDER);
#undef EDGE
        p->lx=axis_value(p->current.sThumbLX,XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
        p->ly=axis_value(p->current.sThumbLY,XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
        p->rx=axis_value(p->current.sThumbRX,XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
        p->ry=axis_value(p->current.sThumbRY,XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
        p->lt=(float)p->current.bLeftTrigger/255.0f; p->rt=(float)p->current.bRightTrigger/255.0f;
        if(p->connected && i>0 && !p->joined && p->pressed_start){p->joined=true;game->players[i].position=BwVec3(game->spawn_point.x+(float)i*1.5f,game->spawn_point.y,game->spawn_point.z);game->players[i].yaw=0.0f;game->players[i].pitch=0.0f;game->players[i].vertical_velocity=0.0f;game->players[i].grounded=true;}
        if(!p->connected && i>0)p->joined=false;
        if(p->connected && p->joined) game->active_slots[count++]=i;
    }
    game->player_count=count;
    if(!count) { game->player_count=1; game->active_slots[0]=0; }
}
