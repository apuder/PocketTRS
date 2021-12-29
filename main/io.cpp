

#include "driver/gpio.h"
#include "trs_screen.h"
#include "trs_memory.h"
#include "cassette.h"
#include "spi.h"
#include "i2s.h"
#include "trs-io.h"
#include "wifi.h"
#include "frehd.h"
#include "config.h"
#include "settings.h"
#include "grafyx.h"


static uint8_t modeimage = 8;
static uint8_t port_0xe0 = 0b11110011;
static uint8_t port_0xec = 0xff;
static int ctrlimage = 0;

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
   case 0x80:
      grafyx_write_x(data);
      return;
    case 0x81:
      grafyx_write_y(data);
      return;
    case 0x82:
      grafyx_write_data(data);
      return;
    case 0x83:
      grafyx_write_mode(data);
      return;
    case 0x84:
    case 0x85:
    case 0x86:
    case 0x87:
      if (trs_model >= 4) {
	      int changes = data ^ ctrlimage;
	      if (changes & 0x80) {
	        mem_video_page((data & 0x80) >> 7);
          printf("mem_video_page: %d\n", (data & 0x80) >> 7);
	      }
	      if (changes & 0x70) {
	        mem_bank((data & 0x70) >> 4);
          printf("mem_bank: %d\n", (data & 0x70) >> 4);
      	}
	      if (changes & 0x08) {
	        trs_screen.setInverse((data & 0x08) >> 3);
	      }
	      if (changes & 0x04) {
          //printf("Switch mode: %d\n", data & 4);
          trs_screen.setMode((data & 0x04) ? MODE_TEXT_80x24 : MODE_TEXT_64x16);
      	}
	      if (changes & 0x03) {
	        mem_map(data & 0x03);
          //printf("mem_map: %d\n", (data & 0x3));
	      }
	      ctrlimage = data;
      }
      break;
    case 0xEC:
      port_0xec = data;
      // Fall through
    case 0xED:
    case 0xEE:
    case 0xEF:
      modeimage = data;
      trs_cassette_motor((modeimage & 0x02) >> 1, z80_state_t_count);
      trs_screen.setExpanded((data & 0x04) >> 2);
      if (trs_model >= 4)
        trs_timer_speed((modeimage & 0x40) >> 6);
      return;
    case 0xF8:
    case 0xF9:
    case 0xFA:
    case 0xFB:
      {
        char buf[2] = {data, 0};
        trs_printer_write(buf);
        return;
      }
    case 0xff:
      trs_cassette_out(data & 3, z80_state_t_count);
      return;
  }
#if 0
  if ((address & 0xc0) == 0xc0) {
    printf("out(0x%02X, 0x%02X)\n", address, data);
  }
#endif

  if (settingsTrsIO.isEnabled()) {
    if ((address & 0xc0) == 0xc0) {
      frehd_out(address, data);
      frehd_check_action();
    } else if (address == 31) {
      if (!TrsIO::outZ80(data)) {
	TrsIO::processInBackground();
	port_0xe0 &= ~(1 << 3);
      }
    }
    // Ignore
    return;
  }

  // Route request to external I/O bus
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
    case 0x82:
      return grafyx_read_data();
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
      if (settingsTrsIO.isEnabled()) {
	return port_0xe0;
      } else {
	// Bit 2 is 0 to signal that a RTC INT happened. See ROM address 0x35D8
	uint8_t b = port_0xe0;
	b |= (readPortExpander(MCP23S08, MCP23S08_GPIO) & TRS_IOBUSINT) ? (1 << 3) : 0;
	return b;
      }
    }
#endif
    case 0xF8:
    case 0xF9:
    case 0xFA:
    case 0xFB:
      return trs_printer_read();
    case 0xff:
      return (modeimage & 0x7e) | trs_cassette_in(z80_state_t_count);
  }
  if ((port_0xec & (1 << 4)) == 0) {
    // I/O disabled
    return 0xff;
  }

  if (settingsTrsIO.isEnabled()) {
    if ((address & 0xc0) == 0xc0) {
      frehd_check_action();
      return frehd_in(address & 0x0f);
    } else if (address == 31) {
      port_0xe0 |= 1 << 3;
      return TrsIO::inZ80();
    }
    return 0xff;
  }

  // Route interaction to external I/O bus

  // Configure port A as input
  set_iodira(0xff);
  // Set the I/O address on port B
  set_gpiob(address);
  // Assert IORQ_N and IN_N
  writePortExpander(MCP23S08, MCP23S08_GPIO, 0xff & ~(TRS_IORQ | TRS_IN));
  while (!(readPortExpander(MCP23S08, MCP23S08_GPIO) & TRS_IOBUSWAIT)) ;
  // Busy wait while IOBUSWAIT_N is asserted
  uint8_t data = readPortExpander(MCP23S17, MCP23S17_GPIOA);
  // Release IORQ_N and IN_N
  writePortExpander(MCP23S08, MCP23S08_GPIO, 0xff);
#if 0
  if ((address & 0xc0) == 0xc0) {
    printf("in(0x%02X): 0x%02X\n", address, data);
  }
#endif
  return data;
}

void init_io()
{
  init_frehd();
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


/*****************************************
 * SettingsTrsIO
 *****************************************/

static const char* KEY_TRS_IO = "trs_io";

static bool setting_trs_io;

void SettingsTrsIO::init()
{
  uint8_t flag = nvs_get_u8(KEY_TRS_IO);
  use_trs_io = (flag != 0);
}

bool SettingsTrsIO::isEnabled()
{
#ifdef CONFIG_POCKET_TRS_TTGO_VGA32_SUPPORT
  return true;
#else
  return use_trs_io;
#endif
}

void SettingsTrsIO::setEnabled(bool flag)
{
  nvs_set_u8(KEY_TRS_IO, flag ? 1 : 0);
  use_trs_io = flag;
}

SettingsTrsIO settingsTrsIO;
