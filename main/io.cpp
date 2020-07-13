

#include <Arduino.h>
#include "driver/gpio.h"
#include "cassette.h"
#include "sound.h"
#include "spi.h"
#include "config.h"


static uint8_t modeimage = 8;
static uint8_t port_0xec = 0xff;


static void set_iodira(uint8_t dir)
{
  static uint8_t current_dir = 0xff;

  if (dir == current_dir) {
    return;
  }
  writePortExpander(MCP23S17, MCP23S17_IODIRA, dir);
  current_dir = dir;
}

static void set_gpiob(uint8_t address)
{
  static uint8_t current_address = 0;

  if (address == current_address) {
    return;
  }
  writePortExpander(MCP23S17, MCP23S17_GPIOB, address);
  current_address = address;
}

void z80_out(uint8_t address, uint8_t data, tstate_t z80_state_t_count)
{
  switch(address) {
    case 0xEC:
      port_0xec = data;
      // Fall through
    case 0xED:
    case 0xEE:
    case 0xEF:
      modeimage = data;
      trs_cassette_motor((modeimage & 0x02) >> 1, z80_state_t_count);
      return;
    case 0xff:
      trs_cassette_out(data & 3, z80_state_t_count);
      transition_out(data, z80_state_t_count);
      return;
  }
#if 1
  if ((address & 0xc0) == 0xc0) {
    Serial.print("out(");
    Serial.print(address);
    Serial.print(",");
    Serial.print(data);
    Serial.println(")");
  }
#endif
#ifndef DISABLE_IO
  // Configure port A as output
  set_iodira(0);
  // Set the I/O address on port B
  set_gpiob(address);
  // Write the data to port A
  writePortExpander(MCP23S17, MCP23S17_GPIOA, data);
  // Assert IORQ_N and OUT_N
  writePortExpander(MCP23S08, MCP23S08_GPIO, ~(TRS_IORQ | TRS_OUT));
  // Busy wait while IOBUSWAIT_N is asserted
  while (!(readPortExpander(MCP23S08, MCP23S08_GPIO) & TRS_IOBUSWAIT)) ;
  // Release IORQ_N and OUT_N
  writePortExpander(MCP23S08, MCP23S08_GPIO, 0xff);
#endif
}

uint8_t z80_in(uint8_t address, tstate_t z80_state_t_count)
{
  switch(address) {
    case 0xec:
      return port_0xec;
#ifdef DISABLE_IO
    case 0xe0:
      // This will signal that a RTC INT happened. See ROM address 0x35D8
      return ~4;
    default:
      return 0xff;
#else
    case 0xe0:
    {
      // Bit 2 is 0 to signal that a RTC INT happened. See ROM address 0x35D8
      uint8_t b = 0b11110011;
      b |= (readPortExpander(MCP23S08, MCP23S08_GPIO) & TRS_IOBUSINT) ? (1 << 3) : 0;
      return b;
    }
#endif
    case 0xff:
      return (modeimage & 0x7e) | trs_cassette_in(z80_state_t_count);
  }
  if ((port_0xec & (1 << 4)) == 0) {
    // I/O disabled
    return 0xff;
  }
  // Configure port A as input
  set_iodira(0xff);
  // Set the I/O address on port B
  set_gpiob(address);
  // Assert IORQ_N and IN_N
  writePortExpander(MCP23S08, MCP23S08_GPIO, ~(TRS_IORQ | TRS_IN));
  while (!(readPortExpander(MCP23S08, MCP23S08_GPIO) & TRS_IOBUSWAIT)) ;
  // Busy wait while IOBUSWAIT_N is asserted
  uint8_t data = readPortExpander(MCP23S17, MCP23S17_GPIOA);
  // Release IORQ_N and IN_N
  writePortExpander(MCP23S08, MCP23S08_GPIO, 0xff);
  #if 1
  if ((address & 0xc0) == 0xc0) {
    Serial.print("in(");
    Serial.print(address);
    Serial.print("): 0x");
    Serial.println(data, HEX);
  }
  #endif
  return data;
}

void init_io()
{
#ifndef DISABLE_IO
  init_spi();
#endif

#if 0
z80_out(0xc5, 3);
for (int i = 0; i < 10; i++) {
  Serial.println(z80_in(0xc4));
}

while(1);
#endif
}
