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
#undef EDGE
        p->lx=axis_value(p->current.sThumbLX,XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
        p->ly=axis_value(p->current.sThumbLY,XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
        p->rx=axis_value(p->current.sThumbRX,XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
        p->ry=axis_value(p->current.sThumbRY,XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
        p->lt=(float)p->current.bLeftTrigger/255.0f; p->rt=(float)p->current.bRightTrigger/255.0f;
        if(p->connected) game->active_slots[count++]=i;
    }
    game->player_count=count?count:1;
    if(!count) game->active_slots[0]=0;
}
