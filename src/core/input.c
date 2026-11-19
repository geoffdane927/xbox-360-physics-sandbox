#include "game/game.h"
static float axis(int16_t value, int16_t deadzone)
{
    if (value > -deadzone && value < deadzone) return 0.0f;
    return bw_clampf((float)value / 32767.0f, -1.0f, 1.0f);
}
void bw_input_poll(BwGame *game)
{
    int count = 0;
    for (int i = 0; i < BW_MAX_PLAYERS; ++i) {
        BwPad *pad = &game->pads[i];
        const uint16_t old = pad->state.gamepad.buttons;
        INPUT_STATE next = {0};
        const uint32_t result = XamInputGetState((uint32_t)i, 0, &next);
        pad->connected = result == ERROR_SUCCESS;
        if (!pad->connected) next = (INPUT_STATE){0};
        pad->previous_buttons = old;
        pad->state = next;
        const uint16_t now = next.gamepad.buttons;
#define BW_EDGE(field, button) pad->field = (now & (button)) != 0 && (old & (button)) == 0
        BW_EDGE(pressed_a, INPUT_GAMEPAD_A);
        BW_EDGE(pressed_b, INPUT_GAMEPAD_B);
        BW_EDGE(pressed_x, INPUT_GAMEPAD_X);
        BW_EDGE(pressed_back, INPUT_GAMEPAD_BACK);
        BW_EDGE(pressed_start, INPUT_GAMEPAD_START);
        BW_EDGE(pressed_guide, INPUT_GAMEPAD_GUIDE);
        BW_EDGE(pressed_left, INPUT_GAMEPAD_DPAD_LEFT);
        BW_EDGE(pressed_right, INPUT_GAMEPAD_DPAD_RIGHT);
#undef BW_EDGE
        pad->lx = axis(next.gamepad.thumb_lx, 7849);
        pad->ly = axis(next.gamepad.thumb_ly, 7849);
        pad->rx = axis(next.gamepad.thumb_rx, 8689);
        pad->ry = axis(next.gamepad.thumb_ry, 8689);
        pad->lt = (float)next.gamepad.left_trigger / 255.0f;
        pad->rt = (float)next.gamepad.right_trigger / 255.0f;
        if (pad->connected) game->active_slots[count++] = i;
    }
    game->player_count = count > 0 ? count : 1;
    if (count == 0) game->active_slots[0] = 0;
}
