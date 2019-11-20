
#ifndef __TRS_H__
#define __TRS_H__

#include <stdint.h>

// Model III specs
#define TIMER_HZ_M3 30
#define CLOCK_MHZ_M3 2.02752
#define RAM_SIZE 48

void poke_mem(uint16_t address, uint8_t data);
uint8_t peek_mem(uint16_t address);
void z80_reset(uint16_t entryAddr);
void z80_run();

#endif
