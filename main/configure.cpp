
#include "settings.h"

extern "C" {
#include "ui.h"
#include "trs-lib.h"
}

static const char* items[] = {
  "WHITE",
  "GREEN",
  "AMBER",
  NULL};

static uint8_t screen_color = 0;

static bool enable_trs_io = true;

static char tz[32 + 1] = "";

static char wifi_ssid[32 + 1] = "";
static char wifi_passwd[32 + 1] = "";

static char smb_url[40 + 1] = "";
static char smb_user[32 + 1] = "";
static char smb_passwd[32 + 1] = "";

static form_item_t configuration_form[14];

static void init_configuration_form()
{
  init_form_begin(configuration_form);
  init_form_header("GENERAL:");
  init_form_checkbox("Enable TRS-IO", &enable_trs_io);
  init_form_select("Screen color", &screen_color, items);
  init_form_input("Timezone", 0, sizeof(tz) - 1, tz);
  init_form_header("");
  init_form_header("WIFI:");
  init_form_input("SSID", 0, sizeof(wifi_ssid) - 1, wifi_ssid);
  init_form_input("Password", 0, sizeof(wifi_passwd) - 1, wifi_passwd);
  init_form_header("");
  init_form_header("SMB:");
  init_form_input("URL", 0, sizeof(smb_url) - 1, smb_url);
  init_form_input("User", 0, sizeof(smb_user) - 1, smb_user);
  init_form_input("Password", 0, sizeof(smb_passwd) - 1, smb_passwd);
  init_form_end(configuration_form);
}

void configure()
{
  init_configuration_form();
  screen_color = (uint8_t) settingsScreen.getScreenColor();
  enable_trs_io = settingsTrsIO.isEnabled();
  form("Configuration", configuration_form, false);
  settingsScreen.setScreenColor((screen_color_t) screen_color);
  settingsTrsIO.setEnabled(enable_trs_io);
}
