
#include "settings.h"
#include "ui.h"
#include "trs-lib.h"

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

static form_item_t configuration_form[] = {
  { FORM_TYPE_HEADER, "GENERAL:" },
  { FORM_TYPE_CHECKBOX, "Enable TRS-IO", .u.checkbox.checked = &enable_trs_io},
  { FORM_TYPE_SELECT, "Screen color", .u.select.selected = &screen_color,
    .u.select.items = &items},
  { FORM_TYPE_INPUT, "Timezone", .u.input.len = sizeof(tz) - 1,
    .u.input.buf = tz, .u.input.width = 0},
  { FORM_TYPE_HEADER, "" },
  { FORM_TYPE_HEADER, "WIFI:" },
  { FORM_TYPE_INPUT, "SSID", .u.input.len = sizeof(wifi_ssid) - 1,
    .u.input.buf = wifi_ssid, .u.input.width = 0},
  { FORM_TYPE_INPUT, "Password", .u.input.len = sizeof(wifi_passwd) - 1,
    .u.input.buf = wifi_passwd, .u.input.width = 0},
  { FORM_TYPE_HEADER, "" },
  { FORM_TYPE_HEADER, "SMB:" },
  { FORM_TYPE_INPUT, "URL", .u.input.len = sizeof(smb_url) - 1,
    .u.input.buf = smb_url, .u.input.width = 0},
  { FORM_TYPE_INPUT, "User", .u.input.len = sizeof(smb_user) - 1,
    .u.input.buf = smb_user, .u.input.width = 0},
  { FORM_TYPE_INPUT, "Password", .u.input.len = sizeof(smb_passwd) - 1,
    .u.input.buf = smb_passwd, .u.input.width = 0},
  { FORM_TYPE_END }
};



void configure()
{
  screen_color = get_setting_screen_color();
  enable_trs_io = get_setting_trs_io();
  form("Configuration", configuration_form, false);
  set_setting_screen_color(screen_color);
  set_setting_trs_io(enable_trs_io);
}
