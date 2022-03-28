
#include "z80.h"
#include "trs_screen.h"
#include "trs_memory.h"
#include "trs.h"
#include "i2s.h"
#include "io.h"
#include <freertos/task.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>


#define CYCLES_PER_TIMER_M3 ((unsigned int) (CLOCK_MHZ_M3 * 1000000 / TIMER_HZ_M3))
#define CYCLES_PER_TIMER_M4 ((unsigned int) (CLOCK_MHZ_M4 * 1000000 / TIMER_HZ_M4))

static unsigned int cycles_per_timer = CYCLES_PER_TIMER_M3;
static unsigned int timer_hz = TIMER_HZ_M3;

int trs_model = 4;

static Z80Context z80ctx;


void trs_timer_speed(int fast)
{
  if (trs_model == 3) fast = 0;
  timer_hz = fast ? TIMER_HZ_M4 : TIMER_HZ_M3;
  cycles_per_timer = fast ? CYCLES_PER_TIMER_M4 : CYCLES_PER_TIMER_M3;
}

void poke_mem(uint16_t address, uint8_t data)
{
  mem_write(address, data);
}

uint8_t peek_mem(uint16_t address)
{
  return mem_read(address);
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
#if 0
    printf("in(0x%02x)\n", address);
#endif
  return z80_in(address & 0xff, total_tstate_count);
}

static void z80_io_write(int param, ushort address, byte data)
{
#if 0
  printf("out(0x%02x): 0x%02x\n", address, data);
#endif
  z80_out(address & 0xff, data, total_tstate_count);
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
  unsigned int curtime;
  unsigned int deltatime;
  static unsigned int lasttime = 0;
  static int count = 0;

  deltatime = 1000 / timer_hz;

  curtime = get_ticks();

  if (lasttime + deltatime > curtime) {
    vTaskDelay((lasttime + deltatime - curtime) / portTICK_PERIOD_MS);
    //if ((count++ % 100) == 0) printf("DELAY: %d\n", (lasttime + deltatime - curtime) / portTICK_PERIOD_MS);
  }
  curtime = get_ticks();

  lasttime += deltatime;
  if ((lasttime + deltatime) < curtime) {
    lasttime = curtime;
  }
}

void z80_reset(ushort entryAddr)
{
  mem_init();
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
  mem_init();
  trs_screen.setMode(MODE_TEXT_64x16);
  trs_screen.setInverse(false);
  trs_screen.refresh();
}

void z80_run()
{
  unsigned last_tstate_count = z80ctx.tstates;
  Z80Execute(&z80ctx);
  total_tstate_count += z80ctx.tstates - last_tstate_count;
  if (z80ctx.tstates >= cycles_per_timer) {
    sync_time_with_host();
    z80ctx.tstates -=  cycles_per_timer;
    z80ctx.int_req = 1;
  }
}
