// What upstream TRS-IO's own app (TRS-IO/src/esp/main) provides to its
// library code, for PocketTRS: there is no FPGA behind TRS-IO's SPI calls
// here, the Z80 is emulated. Most FPGA calls do nothing; "TRS-IO command
// done" goes to the emulator (trs_io_host_set_done_handler). Settings are
// upstream's own settings.cpp, compiled by CMakeLists.txt.
//
// Not here, because PocketTRS's main component implements them itself:
// init_spi() (spi.cpp), set_led() (led.cpp), and XRAY's
// spi_xram_*() / spi_*_breakpoint() (xray.cpp).

#include "trs_io_host.h"

#include "esp_log.h"
#include "esp_system.h"
#include <string.h>

#include "event.h"
#include "http.h"
#include "keyb.h"
#include "led.h"
#include "ntp_sync.h"
#include "ota_stateful.h"
#include "rst.h"
#include "settings.h"
#include "spi.h"
#include "spiffs.h"
#include "trs-fs.h"
#include "wifi.h"

// upstream's init_settings(), renamed by CMakeLists.txt: PocketTRS has its own.
void trs_io_init_settings();

static void (*done_handler)(void) = NULL;
static void (*screen_color_handler)(uint8_t) = NULL;
// TRS-IO's settings are loaded (settings_commit() aborts before that).
static bool settings_loaded = false;

extern "C" void trs_io_host_init(void)
{
  trs_io_init_settings();
  settings_loaded = true;
}

extern "C" void trs_io_host_start_network(void)
{
  init_spiffs();
  init_wifi();
  init_http();
}

extern "C" void trs_io_host_poll(void)
{
  check_events();
}

extern "C" const char *trs_io_host_wifi_ssid(void)
{
  return settings_get_wifi_ssid().c_str();
}

extern "C" void trs_io_host_get_config(trs_io_host_config_t *config)
{
  strlcpy(config->tz, settings_get_tz().c_str(), sizeof(config->tz));
  strlcpy(config->ssid, settings_get_wifi_ssid().c_str(), sizeof(config->ssid));
  strlcpy(config->passwd, settings_get_wifi_passwd().c_str(), sizeof(config->passwd));
  strlcpy(config->smb_url, settings_get_smb_url().c_str(), sizeof(config->smb_url));
  strlcpy(config->smb_user, settings_get_smb_user().c_str(), sizeof(config->smb_user));
  strlcpy(config->smb_passwd, settings_get_smb_passwd().c_str(), sizeof(config->smb_passwd));
}

extern "C" void trs_io_host_set_tz(const char *tz)
{
  settings_set_tz(tz);
  settings_commit();
  set_timezone();
}

extern "C" void trs_io_host_set_smb(const char *url, const char *user, const char *passwd)
{
  settings_set_smb_url(url);
  settings_set_smb_user(user);
  settings_set_smb_passwd(passwd);
  settings_commit();
  init_trs_fs_smb();
}

extern "C" void trs_io_host_set_wifi(const char *ssid, const char *passwd)
{
  settings_set_wifi_ssid(ssid);
  settings_set_wifi_passwd(passwd);
  settings_commit();
}

extern "C" void trs_io_host_reset_settings(void)
{
  settings_reset_all();
}

extern "C" void trs_io_host_set_screen_color(uint8_t color)
{
  if (settings_loaded && settings_get_screen_color() != color) {
    settings_set_screen_color(color);
    settings_commit();
  }
}

extern "C" void trs_io_host_set_screen_color_handler(void (*handler)(uint8_t color))
{
  screen_color_handler = handler;
}

extern "C" void trs_io_host_set_done_handler(void (*done)(void))
{
  done_handler = done;
}

//----------------------------------------------------------------
// version.cpp, which upstream generates: TRS-IO's revision (CMakeLists.txt)

const char* GIT_REV = TRS_IO_GIT_REV;
const char* GIT_TAG = TRS_IO_GIT_TAG;
const char* GIT_BRANCH = TRS_IO_GIT_BRANCH;

//----------------------------------------------------------------
// keyb.h: a PS/2 keyboard on TRS-IO++. The web UI's layout setting is
// stored, nothing more.

void set_keyb_layout() {}

//----------------------------------------------------------------
// rst.h

void reboot_trs_io()
{
  esp_restart();
}

//----------------------------------------------------------------
// led.h: TRS-IO's RGB status LED. set_led() is PocketTRS's (main/led.cpp).

void init_led() {}

//----------------------------------------------------------------
// spi.h: TRS-IO's FPGA. Nothing behind it here; reads return 0, and
// only the screen color and "command done" go anywhere.

uint8_t spi_get_cookie() { return 0; }
uint8_t spi_get_fpga_version() { return 0; }
uint8_t spi_get_mode() { return 0; }
void spi_bram_poke(uint16_t addr, uint8_t data) {}
uint8_t spi_bram_peek(uint16_t addr) { return 0; }
uint8_t spi_dbus_read() { return 0; }
void spi_dbus_write(uint8_t d) {}
uint8_t spi_abus_read() { return 0; }
void spi_xray_resume() {}
void spi_set_full_addr(bool flag) {}
// The web UI's phosphor color: TRS-IO has already stored it; the emulator
// draws the screen, so it gets told.
void spi_set_screen_color(uint8_t color)
{
  if (screen_color_handler != NULL) {
    screen_color_handler(color);
  }
}
void spi_set_printer_en(bool enable) {}
void spi_set_audio_output(uint8_t audio_output) {}
void spi_send_keyb(uint8_t idx, uint8_t mask) {}
void spi_ptrs_rst() {}
void spi_z80_pause() {}
void spi_z80_resume() {}
void spi_z80_dsp_set_addr(uint16_t addr) {}
void spi_z80_dsp_poke(uint8_t v) {}
uint8_t spi_z80_dsp_peek() { return 0; }
void spi_set_activity_led(bool bottom, bool top) {}
void spi_set_led(bool r, bool g, bool b) {}
uint8_t spi_get_config() { return 0; }
void spi_set_cass_in() {}
void spi_set_spi_ctrl_reg(uint8_t reg) {}
void spi_set_spi_data(uint8_t data) {}
uint8_t spi_get_spi_data() { return 0; }
void spi_set_esp_status(uint8_t status) {}

// The one FPGA command that matters: the Z80 waits for it.
void spi_trs_io_done()
{
  if (done_handler != NULL) {
    done_handler();
  }
}

//----------------------------------------------------------------
// ota_stateful.h: firmware update by uploading a tar in the web UI. Its
// tar holds TRS-IO's firmware, not PocketTRS's, so refuse it.

void extract_tar_begin(struct extract_tar_context *ctx)
{
  ESP_LOGW("trs-io", "firmware update from the web UI is not supported on PocketTRS");
}

extract_tar_error extract_tar_handle_byte(struct extract_tar_context *ctx, uint8_t byte)
{
  return ete_firmware_update_failed;
}

extract_tar_error extract_tar_end(struct extract_tar_context *ctx)
{
  return ete_ok;
}
