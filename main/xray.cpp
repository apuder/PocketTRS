
#include "xray.h"
#include "spi.h"
#include "retrostore.h"
#include <assert.h>


#define MAX_BREAKPOINTS 16

static uint16_t breakpoints[MAX_BREAKPOINTS];
static uint16_t active_breakpoints = 0;

static uint8_t xram_data[256];
static uint8_t xram_code[256];


#define STATE_XRAY_RUN 0
#define STATE_XRAY_STOP 1
#define STATE_XRAY_RESUME 2 // Not used in emulator

static int state_xray = STATE_XRAY_RUN;

static uint16_t xray_base_addr;

static int breakpoint_idx;

extern retrostore::RsSystemState trs_state;

// Defined in trs_memory.cpp
void mem_write(unsigned int address, int value);

bool xray_mem_read(uint16_t addr, uint8_t* byte)
{
  if (state_xray == STATE_XRAY_RUN) {
    if (active_breakpoints == 0) {
        return false;
    }
    for (int i = 0; i < MAX_BREAKPOINTS; i++) {
        if (active_breakpoints & (1 << i)) {
            if (breakpoints[i] == addr) {
                state_xray = STATE_XRAY_STOP;
                xray_base_addr = addr;
                breakpoint_idx = i;
                *byte = xram_code[0];
                return true;
            }
        }
    }
    return false;
  }

  // Must be STATE_XRAY_STOP
  if ((addr & 0xff00) == 0xff00) {
    *byte = xram_data[addr & 0xff];
  } else {
    *byte = xram_code[addr - xray_base_addr];
  }

  if (addr == xray_base_addr) {
    state_xray = STATE_XRAY_RUN;
    spi_clear_breakpoint(breakpoint_idx);
    uint8_t* buf = trs_state.regions[1].data.get();
    for (int i = 0; i < 32768; i++) {
      mem_write(0x8000 + i, *buf++);
    }
    return false;
  }

  return true;
}

bool xray_mem_write(uint16_t addr, uint8_t byte)
{
    if (state_xray == STATE_XRAY_RUN) {
        return false;
    }
    assert((addr & 0xff00) == 0xff00);
    xram_data[addr & 0xff] = byte;
    return true;
}

void spi_xram_poke_code(uint8_t addr, uint8_t data)
{
  assert(addr < 256);
  xram_code[addr] = data;
}

void spi_xram_poke_data(uint8_t addr, uint8_t data)
{
  assert(addr < 256);
  xram_data[addr] = data;
}

uint8_t spi_xram_peek_data(uint8_t addr)
{
  assert(addr < 256);
  return xram_code[addr];
}

void spi_set_breakpoint(uint8_t n, uint16_t addr)
{
  assert(n < MAX_BREAKPOINTS);
  breakpoints[n] = addr;
  active_breakpoints |= 1 << n;
}

void spi_clear_breakpoint(uint8_t n)
{
  assert(n < MAX_BREAKPOINTS);
  active_breakpoints &= ~(1 << n);
}
