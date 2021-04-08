#pragma once

#include "esp_system.h"

void init_button();
bool is_button_long_press();
bool is_button_short_press();
bool is_button_pressed();