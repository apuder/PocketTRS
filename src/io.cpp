

#include <Arduino.h>
#include "driver/gpio.h"
#include "cassette.h"
#include "sound.h"
#include "i2c.h"
#include "config.h"


static uint8_t modeimage = 8;
static uint8_t port_0xec = 0xff;


static void set_iodirb(uint8_t dir)
{
  static uint8_t current_dir = 0xff;

  if (dir == current_dir) {
    return;
  }
  writeMCP(MCP23017_ADDRESS, MCP23017_IODIRB, dir);
  current_dir = dir;
}

static void set_gpioa(uint8_t address)
{
  static uint8_t current_address = 0;

  if (address == current_address) {
    return;
  }
  writeMCP(MCP23017_ADDRESS, MCP23017_GPIOA, address);
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
      return;
    case 0xff:
      transition_out(data, z80_state_t_count);
      break;

  }
  #if 0
  if (address == 31) {
    Serial.print("out(");
    Serial.print(address);
    Serial.print(",");
    Serial.print(data);
    Serial.println(")");
  }
  #endif
#ifndef DISABLE_IO
  // Configure port B as output
  set_iodirb(0);
  // Set the I/O address on port A
  set_gpioa(address);
  // Write the data to port B
  writeMCP(MCP23017_ADDRESS, MCP23017_GPIOB, data);
  // Assert IORQ_N and OUT_N
  writeMCP(MCP23008_ADDRESS, MCP23008_GPIO, ~(TRS_IORQ | TRS_OUT));
  // Busy wait while IOBUSWAIT_N is asserted
  while (!(readMCP(MCP23008_ADDRESS, MCP23008_GPIO) & TRS_IOBUSWAIT)) ;
  // Release IORQ_N and OUT_N
  writeMCP(MCP23008_ADDRESS, MCP23008_GPIO, 0xff);
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
      b |= (readMCP(MCP23008_ADDRESS, MCP23008_GPIO) & TRS_IOBUSINT) ? (1 << 3) : 0;
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
  // Configure port B as input
  set_iodirb(0xff);
  // Set the I/O address on port A
  set_gpioa(address);
  // Assert IORQ_N and IN_N
  writeMCP(MCP23008_ADDRESS, MCP23008_GPIO, ~(TRS_IORQ | TRS_IN));
  while (!(readMCP(MCP23008_ADDRESS, MCP23008_GPIO) & TRS_IOBUSWAIT)) ;
  // Busy wait while IOBUSWAIT_N is asserted
  uint8_t data = readMCP(MCP23017_ADDRESS, MCP23017_GPIOB);
  // Release IORQ_N and IN_N
  writeMCP(MCP23008_ADDRESS, MCP23008_GPIO, 0xff);
  #if 0
  if (address == 31) {
    Serial.print("in(");
    Serial.print(address);
    Serial.print("): 0x");
    Serial.println(data, HEX);
  }
  #endif
  return data;
}

void init_i2c();

void init_io()
{
#ifndef DISABLE_IO
  init_i2c();
#endif

#if 0
z80_out(0xc5, 3);
for (int i = 0; i < 10; i++) {
  Serial.println(z80_in(0xc4));
}

while(1);
#endif
}
