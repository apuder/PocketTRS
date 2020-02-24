
#pragma once

//#define DISABLE_IO 1

// VGA
#define VGA_RED GPIO_NUM_22
#define VGA_GREEN GPIO_NUM_21
#define VGA_BLUE GPIO_NUM_23
#define VGA_HSYNC GPIO_NUM_26
#define VGA_VSYNC GPIO_NUM_5

#define TRS_CASSETTE_IN GPIO_NUM_36

// Keyboard: GPIO 32 & 33

// Audio: 25

// Bits of MCP23008
#define TRS_IN (1 << 0)
#define TRS_OUT (1 << 1)
#define TRS_IOBUSINT (1 << 2)
#define TRS_IOBUSWAIT (1 << 3)
#define TRS_EXTIOSEL (1 << 4)
#define TRS_M1 (1 << 5)
#define TRS_IORQ (1 << 6)
