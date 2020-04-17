
#pragma once

#include "trs.h"

void trs_cassette_motor(int value, tstate_t z80_state_t_count);
void trs_cassette_out(int value, tstate_t z80_state_t_count);
int trs_cassette_in(tstate_t z80_state_t_count);
void init_cassette_in();
