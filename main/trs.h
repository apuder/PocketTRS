
#ifndef __TRS_H__
#define __TRS_H__

#include <stdint.h>

typedef unsigned long long tstate_t;

// Model III specs
#define TIMER_HZ_M3 30
#define CLOCK_MHZ_M3 2.02752

extern int trs_model;

void poke_mem(uint16_t address, uint8_t data);
uint8_t peek_mem(uint16_t address);
void z80_reset(uint16_t entryAddr);
void z80_reset();
void z80_run();

#endif
