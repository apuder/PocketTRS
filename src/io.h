
#pragma once

#include <Arduino.h>

void z80_out(uint8_t address, uint8_t data);
uint8_t z80_in(uint8_t address);

void init_io();
