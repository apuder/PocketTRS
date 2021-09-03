
#ifndef __SPI_H__
#define __SPI_H__

#include <stdint.h>

#define MCP23S17 1
#define MCP23S08 0

/*
 * MCP23S17
 */
#define MCP23S17_IODIRA 0x00
#define MCP23S17_IPOLA 0x02
#define MCP23S17_GPINTENA 0x04
#define MCP23S17_DEFVALA 0x06
#define MCP23S17_INTCONA 0x08
#define MCP23S17_IOCONA 0x0A
#define MCP23S17_GPPUA 0x0C
#define MCP23S17_INTFA 0x0E
#define MCP23S17_INTCAPA 0x10
#define MCP23S17_GPIOA 0x12
#define MCP23S17_OLATA 0x14


#define MCP23S17_IODIRB 0x01
#define MCP23S17_IPOLB 0x03
#define MCP23S17_GPINTENB 0x05
#define MCP23S17_DEFVALB 0x07
#define MCP23S17_INTCONB 0x09
#define MCP23S17_IOCONB 0x0B
#define MCP23S17_GPPUB 0x0D
#define MCP23S17_INTFB 0x0F
#define MCP23S17_INTCAPB 0x11
#define MCP23S17_GPIOB 0x13
#define MCP23S17_OLATB 0x15

/*
 * MCP23S08
 */
#define MCP23S08_IODIR 0x00
#define MCP23S08_IPOL 0x01
#define MCP23S08_GPINTEN 0x02
#define MCP23S08_DEFVAL 0x03
#define MCP23S08_INTCON 0x04
#define MCP23S08_IOCON 0x05
#define MCP23S08_GPPU 0x06
#define MCP23S08_INTF 0x07
#define MCP23S08_INTCAP 0x08
#define MCP23S08_GPIO 0x09
#define MCP23S08_OLAT 0x0a

#define MCP23S08_ODR (1 << 2)


#define ACK_CHECK_EN 0x1
#define ACK_CHECK_DIS 0x0
#define ACK_VAL 0x0
#define NACK_VAL 0x1

void writePortExpander(uint8_t addr, uint8_t cmd, uint8_t data);
uint8_t readPortExpander(uint8_t addr, uint8_t reg);
void writeDigiPot(uint8_t pot, uint8_t step);

void init_spi();

#endif
