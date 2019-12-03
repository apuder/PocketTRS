
#pragma once

// Pin connected to ST_CP of 74HC595
#define SHIFTER_LATCH GPIO_NUM_27
// Pin connected to SH_CP of 74HC595
#define SHIFTER_CLOCK GPIO_NUM_4
// Pin connected to DS of 74HC595
#define SHIFTER_DATA GPIO_NUM_2

#define TRS_IOBUSINT GPIO_NUM_36
#define TRS_IOBUSWAIT GPIO_NUM_39

// Bits of first shifter
#define TRS_IN (1 << 0)
#define TRS_OUT (1 << 1)
#define TRS_M1 (1 << 2)
#define TRS_IORQ (1 << 3)
#define TRS_ENEXTIO (1 << 4)
