#pragma once

// PocketTRS's side of TRS-IO. Upstream TRS-IO is an app for its own boards
// (TRS-IO/src/esp/main): settings, the FPGA over SPI, LEDs, buttons. The
// trs-io component compiles TRS-IO's library parts and takes the board parts
// from host/ (see host/host.cpp); this is what the rest of PocketTRS calls.

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Load TRS-IO's settings (Wi-Fi, SMB share, time zone, ...; NVS namespace
// "retrostore"). Call once, before init_wifi() or anything else of TRS-IO
// that reads them.
void trs_io_host_init(void);

// TRS-IO's network services: the web UI's SPIFFS partition, Wi-Fi (station
// with TRS-IO's credentials, else its open "TRS-IO" access point) and the
// web server. Needs trs_io_host_init() first.
void trs_io_host_start_network(void);

// What TRS-IO's main loop does besides serving the TRS-80: once Wi-Fi is up,
// mount the SMB share and start the web server. Call every few hundred ms
// from a task with room on its stack (an SMB mount runs on it).
void trs_io_host_poll(void);

// The Wi-Fi network TRS-IO is configured for ("" if none).
const char *trs_io_host_wifi_ssid(void);

// TRS-IO's settings as editable C strings (PocketTRS Configuration). The setters
// store the new values in NVS (a flash write) and apply them: the time zone
// at once, the SMB share by remounting it. New Wi-Fi credentials take effect
// at the next boot.
typedef struct {
  char tz[32 + 1];
  char ssid[32 + 1];
  char passwd[32 + 1];
  char smb_url[100 + 1];
  char smb_user[32 + 1];
  char smb_passwd[32 + 1];
} trs_io_host_config_t;

void trs_io_host_get_config(trs_io_host_config_t *config);
void trs_io_host_set_tz(const char *tz);
void trs_io_host_set_smb(const char *url, const char *user, const char *passwd);
void trs_io_host_set_wifi(const char *ssid, const char *passwd);

// Erase all of TRS-IO's settings (factory reset).
void trs_io_host_reset_settings(void);

// Screen color (0 white, 1 green, 2 amber), shared with the web UI.
// trs_io_host_set_screen_color() stores it as TRS-IO's setting (only if it
// changed; ignored before trs_io_host_init()). The handler is called when
// the web UI picks a color.
void trs_io_host_set_screen_color(uint8_t color);
void trs_io_host_set_screen_color_handler(void (*handler)(uint8_t color));

// A TRS-IO command from the Z80 can finish later, from another task (the
// web UI's file transfer does). The emulator registers how to tell the Z80
// that the current command is done.
void trs_io_host_set_done_handler(void (*done)(void));

#ifdef __cplusplus
}
#endif
