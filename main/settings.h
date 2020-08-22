
#pragma once

#include <inttypes.h>
#include <stdbool.h>

#define SCREEN_COLOR_WHITE 0
#define SCREEN_COLOR_GREEN 1
#define SCREEN_COLOR_AMBER 2

#ifdef __cplusplus
extern "C" {
#endif

void init_settings();
void reset_settings();
uint8_t get_setting_screen_color();
void set_setting_screen_color(uint8_t color);
bool get_setting_trs_io();
void set_setting_trs_io(bool flag);

#ifdef __cplusplus
}
#endif
