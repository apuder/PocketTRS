
#pragma once

#include <inttypes.h>

#define SCREEN_COLOR_WHITE 0
#define SCREEN_COLOR_GREEN 1
#define SCREEN_COLOR_AMBER 2

#ifdef __cplusplus
extern "C" {
#endif

void init_settings();
uint8_t get_screen_color();
void set_screen_color(uint8_t color);

#ifdef __cplusplus
}
#endif
