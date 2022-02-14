
#include "settings.h"
#include "wifi.h"
#include "trs-fs.h"
#include "ntp_sync.h"
#include "esp_event_loop.h"

extern "C" {
#include "ui.h"
#include "trs-lib.h"
}

static const char* screen_color_items[] = {
  "WHITE",
  "GREEN",
  "AMBER",
  NULL};

static uint8_t screen_color = 0;

static const char* rom_items[] = {
  "FreHD",
  "XROM",
  NULL};

static uint8_t rom_type = 0;

static bool show_splash_screen = true;

static bool enable_trs_io = true;

static form_item_t configuration_form[14];

void configure()
{
  trs_io_wifi_config_t* config = get_wifi_config();
  init_form_begin(configuration_form);
  init_form_header("GENERAL:");
  init_form_checkbox("Show splash screen", &show_splash_screen);
#ifndef CONFIG_POCKET_TRS_TTGO_VGA32_SUPPORT
  init_form_checkbox("Enable TRS-IO", &enable_trs_io);
#endif
  form_item_t* rom = init_form_select("ROM", &rom_type, rom_items);
  init_form_select("Screen color", &screen_color, screen_color_items);
  form_item_t* tz = init_form_input("Timezone", 0, MAX_LEN_TZ, config->tz);
  init_form_header("WIFI:");
  form_item_t* ssid = init_form_input("SSID", 0, MAX_LEN_SSID, config->ssid);
  form_item_t* passwd = init_form_input("Password", 0, MAX_LEN_PASSWD, config->passwd);
  init_form_header("SMB:");
  form_item_t* smb_url = init_form_input("URL", 40, MAX_LEN_SMB_URL, config->smb_url);
  form_item_t* smb_user = init_form_input("User", 0, MAX_LEN_SMB_USER, config->smb_user);
  form_item_t* smb_passwd = init_form_input("Password", 0, MAX_LEN_SMB_PASSWD, config->smb_passwd);
#ifdef CONFIG_POCKET_TRS_TTGO_VGA32_SUPPORT
  init_form_header("");
#endif
  init_form_end(configuration_form);
  screen_color = (uint8_t) settingsScreen.getScreenColor();
  rom_type = (uint8_t) settingsROM.getROMType();
  show_splash_screen = !settingsSplashScreen.hideSplashScreen();
  enable_trs_io = settingsTrsIO.isEnabled();

  form("Configuration", configuration_form, false);

  settingsScreen.setScreenColor((screen_color_t) screen_color);
  settingsSplashScreen.hideSplashScreen(!show_splash_screen);
  settingsROM.setROMType((rom_type_t) rom_type);
#ifndef CONFIG_POCKET_TRS_TTGO_VGA32_SUPPORT
  settingsTrsIO.setEnabled(enable_trs_io);
#endif

  if (smb_url->dirty || smb_user->dirty || smb_passwd->dirty) {
    init_trs_fs_smb(config->smb_url, config->smb_user, config->smb_passwd);
  }

  if (tz->dirty) {
    set_timezone(config->tz);
  }

  if (ssid->dirty || passwd->dirty || rom->dirty) {
    wnd_popup("Rebooting PocketTRS...");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    set_wifi_credentials(config->ssid, config->passwd);
  }
}

