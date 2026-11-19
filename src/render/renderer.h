#ifndef BW_RENDERER_H
#define BW_RENDERER_H
#include <stdint.h>
#include "game/game.h"
int bw_renderer_open(uint32_t *width, uint32_t *height);
void bw_renderer_draw(const BwGame *game, uint32_t width, uint32_t height);
void bw_renderer_close(void);
#endif
