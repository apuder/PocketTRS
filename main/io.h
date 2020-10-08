
#pragma once

#include "trs.h"

void z80_out(uint8_t address, uint8_t data, tstate_t z80_state_t_count);
uint8_t z80_in(uint8_t address, tstate_t z80_state_t_count);

void init_io();
