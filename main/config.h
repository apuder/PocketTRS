
#pragma once

//#define DISABLE_IO 1

// SPI
#define SPI_HOST HSPI_HOST
#define SPI_SPEED_MHZ 10

#define SPI_PIN_NUM_MISO GPIO_NUM_19
#define SPI_PIN_NUM_MOSI GPIO_NUM_13
#define SPI_PIN_NUM_CLK GPIO_NUM_14
#define SPI_PIN_NUM_CS_MCP23S17 GPIO_NUM_15
#define SPI_PIN_NUM_CS_MCP23S08 GPIO_NUM_18
#define SPI_PIN_NUM_CS_MCP4351 GPIO_NUM_4
#define SPI_PIN_NUM_INT GPIO_NUM_27


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
