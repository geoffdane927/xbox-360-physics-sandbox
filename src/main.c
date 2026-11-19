#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "game/game.h"
#include "render/renderer.h"
#include "xsdk/kernel.h"
#include "xsdk/log.h"
#include "xsdk/splash.h"

#define BW_FRAME_DT (1.0f/60.0f)

int main(void)
{
    BwGame game;
    memset(&game,0,sizeof(game));
    game.editor_mode=true;
    game.show_grid=true;
    game.quality_scale=1.0f;
    game.render_distance=120.0f;
    for(int i=0;i<BW_MAX_PLAYERS;++i) {
        game.players[i].position=(BwVec3){((float)i-1.5f)*1.2f,0.0f,-8.0f};
        game.players[i].grounded=true;
        game.active_slots[i]=i;
    }
    bw_world_init(&game);
    xsdk_log_write("BlockWorks 360: XSDK modular build; Back toggles editor/player, press Back again to exit is not implemented yet");
    const NTSTATUS splash=xsdk_splash_show(1000);
    if(!NT_SUCCESS(splash)) xsdk_log_write("main: splash returned %08X",(unsigned)splash);
    uint32_t width=0,height=0;
    const int opened=bw_renderer_open(&width,&height);
    if(opened!=0) {
        xsdk_log_write("main: renderer open failed %08X",(unsigned)opened);
        return 1;
    }
    xsdk_log_write("BlockWorks: display %ux%u; press Back to leave",width,height);
    bool running=true;
    while(running) {
        bw_input_poll(&game);
        for(int i=0;i<BW_MAX_PLAYERS;++i) {
            if(game.pads[i].pressed_back) {
                if(!game.editor_mode) running=false;
                else game.editor_mode=false;
                break;
            }
        }
        if(running) {
            bw_players_update(&game,BW_FRAME_DT);
            game.render_distance=120.0f*(game.player_count==1?1.0f:(game.player_count==2?0.72f:0.48f))*game.quality_scale;
            bw_renderer_draw(&game,width,height);
        }
    }
    bw_renderer_close();
    xsdk_log_write("BlockWorks: clean exit");
    return 0;
}
