
#include "z80.h"
#include "trs_screen.h"
#include "trs.h"
#include "i2s.h"
#include "io.h"
#include <freertos/task.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>


#include "rom/model3-frehd.cpp-inc"

#define CYCLES_PER_TIMER ((unsigned int) (CLOCK_MHZ_M3 * 1000000 / TIMER_HZ_M3))

static Z80Context z80ctx;

static byte ram[64 * 1024 - model3_frehd_rom_len];

void poke_mem(uint16_t address, uint8_t data)
{
  if (address < model3_frehd_rom_len) {
    // Trying to write to ROM
    return;
  }
  ram[address - model3_frehd_rom_len] = data;
  if ((address >= 0x3c00) && (address < (0x3c00 + 64 * 16))) {
    // Video RAM access
    trs_screen.drawChar(address - 0x3c00, data);
  }
}

uint8_t peek_mem(uint16_t address)
{
  if (address < model3_frehd_rom_len) {
    // Read from ROM
    return model3_frehd_rom[address];
  } else {
    // Access RAM
    return ram[address - model3_frehd_rom_len];
  }
}

//------------------------------------------------------------------

static tstate_t total_tstate_count = 0;

static byte z80_mem_read(int param, ushort address)
{
  return peek_mem(address);
}

void z80_mem_write(int param, ushort address, byte data)
{
  poke_mem(address, data);
}

static byte z80_io_read(int param, ushort address)
{
  address &= 0xff;
  switch(address) {
  case 31:
  case 0xc0:
  case 0xc1:
  case 0xc2:
  case 0xc3:
  case 0xc4:
  case 0xc5:
  case 0xc6:
  case 0xc7:
  case 0xc8:
  case 0xc9:
  case 0xca:
  case 0xcb:
  case 0xcc:
  case 0xcd:
  case 0xce:
  case 0xcf:
  case 0xe0:
  case 0xec:
  case 0xff:
    return z80_in(address, total_tstate_count);
  default:
#if 0
    Serial.print("in(");
    Serial.print(address);
    Serial.println(")");
#endif
    return 255;
  }
}

static void z80_io_write(int param, ushort address, byte data)
{
  address &= 0xff;
  switch (address) {
  case 31:
  case 0xc0:
  case 0xc1:
  case 0xc2:
  case 0xc3:
  case 0xc4:
  case 0xc5:
  case 0xc6:
  case 0xc7:
  case 0xc8:
  case 0xc9:
  case 0xca:
  case 0xcb:
  case 0xcc:
  case 0xcd:
  case 0xce:
  case 0xcf:
  case 0xec:
  case 0xed:
  case 0xee:
  case 0xef:
  case 0xff:
    z80_out(address, data, total_tstate_count);
    break;
  default:
#if 0
    Serial.print("out(");
    Serial.print(address);
    Serial.print(",");
    Serial.print(data);
    Serial.println(")");
#endif
    break;
  }
}

static int get_ticks()
{
  static struct timeval start_tv, now;
  static int init = 0;

  if (!init) {
    gettimeofday(&start_tv, NULL);
    init = 1;
  }

  gettimeofday(&now, NULL);
  return (now.tv_sec - start_tv.tv_sec) * 1000 +
                 (now.tv_usec - start_tv.tv_usec) / 1000;
}

static void sync_time_with_host()
{
  int curtime;
  int deltatime;
  static int lasttime = 0;

  deltatime = 1000 / TIMER_HZ_M3;

  curtime = get_ticks();

  if (lasttime + deltatime > curtime) {
    vTaskDelay((lasttime + deltatime - curtime) / portTICK_PERIOD_MS);
  }
  curtime = get_ticks();

  lasttime += deltatime;
  if ((lasttime + deltatime) < curtime) {
    lasttime = curtime;
  }
}

void z80_reset(ushort entryAddr)
{
  memset((void*) ram, 0, sizeof(ram));
  memset(&z80ctx, 0, sizeof(Z80Context));
  Z80RESET(&z80ctx);
  z80ctx.PC = entryAddr;
  z80ctx.memRead = z80_mem_read;
  z80ctx.memWrite = z80_mem_write;
  z80ctx.ioRead = z80_io_read;
  z80ctx.ioWrite = z80_io_write;
}

void z80_reset()
{
  Z80RESET(&z80ctx);
}

void z80_run()
{
  unsigned last_tstate_count = z80ctx.tstates;
  Z80Execute(&z80ctx);
  total_tstate_count += z80ctx.tstates - last_tstate_count;
  if (z80ctx.tstates >= CYCLES_PER_TIMER) {
    sync_time_with_host();
    z80ctx.tstates -=  CYCLES_PER_TIMER;
    z80ctx.int_req = 1;
  }
}

void init_trs()
{
  ScreenBuffer* screenBuffer =
    new ScreenBuffer(&ram[0x3c00 - model3_frehd_rom_len], 64, 16);
  trs_screen.push(screenBuffer);
}
