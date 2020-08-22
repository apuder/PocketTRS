
#include "settings.h"
#include "spi.h"
#include <nvs_flash.h>

static nvs_handle storage;

/*****************************************
 * Screen color
 *****************************************/

#define KEY_COLOR "color"

static const uint8_t wiper_settings[][3] = {
  {225, 225, 255},
  {51, 255, 51},
  {255, 177, 0}};

static uint8_t setting_color;

static void init_setting_color()
{
  setting_color = 0;
  esp_err_t err = nvs_get_u8(storage, KEY_COLOR, &setting_color);
  assert(err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND);
  set_setting_screen_color(setting_color);
}

uint8_t get_setting_screen_color()
{
  return setting_color;
}

void set_setting_screen_color(uint8_t color)
{
  for (int i = 0; i < 3; i++) {
    writeDigiPot(i, wiper_settings[color][i]);
  }
  ESP_ERROR_CHECK(nvs_set_u8(storage, KEY_COLOR, color));
  ESP_ERROR_CHECK(nvs_commit(storage));
  setting_color = color;
}

/*****************************************
 * Use TRS-IO
 *****************************************/

#define KEY_TRS_IO "trs_io"

static bool setting_trs_io;

static void init_setting_trs_io()
{
  uint8_t flag = 0;
  esp_err_t err = nvs_get_u8(storage, KEY_TRS_IO, &flag);
  assert(err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND);
  setting_trs_io = (flag != 0);
}

bool get_setting_trs_io()
{
  return setting_trs_io;
}

void set_setting_trs_io(bool flag)
{
  ESP_ERROR_CHECK(nvs_set_u8(storage, KEY_TRS_IO, flag ? 1 : 0));
  ESP_ERROR_CHECK(nvs_commit(storage));
  setting_trs_io = flag;
}

/* Init */
void init_settings()
{
  // Initialize NVS
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  err = nvs_open("ptrs", NVS_READWRITE, &storage);
  ESP_ERROR_CHECK(err);

  init_setting_color();
  init_setting_trs_io();
}

void reset_settings()
{
  ESP_ERROR_CHECK(nvs_erase_all(storage));
  ESP_ERROR_CHECK(nvs_commit(storage));
}
