
#pragma once

// VGA
#define VGA_RED GPIO_NUM_22
#define VGA_GREEN GPIO_NUM_21
#define VGA_BLUE GPIO_NUM_23
#define VGA_HSYNC GPIO_NUM_26
#define VGA_VSYNC GPIO_NUM_5

// Pin connected to ST_CP of 74HC595
#define SHIFTER_LATCH GPIO_NUM_27
// Pin connected to SH_CP of 74HC595
#define SHIFTER_CLOCK GPIO_NUM_4
// Pin connected to DS of 74HC595
#define SHIFTER_DATA GPIO_NUM_2

#define TRS_IOBUSINT GPIO_NUM_34
#define TRS_IOBUSWAIT GPIO_NUM_39

#define TRS_CASSETTE_IN GPIO_NUM_36

// Keyboard: GPIO 32 & 33

// VGA: GPIO_NUM_22, GPIO_NUM_21, GPIO_NUM_23, GPIO_NUM_26, GPIO_NUM_5

// Audio: 25

// Bits of first shifter
#define TRS_IN (1 << 0)
#define TRS_OUT (1 << 1)
#define TRS_M1 (1 << 2)
#define TRS_IORQ (1 << 3)
#define TRS_ENEXTIO (1 << 4)

#define DELAY_IOBUSWAIT 5000
