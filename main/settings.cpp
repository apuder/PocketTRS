
#include "settings.h"
#include "spi.h"
#include <nvs_flash.h>

static nvs_handle storage;

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

  set_screen_color(get_screen_color());
}

/*****************************************
 * Screen color
 *****************************************/

#define KEY_COLOR "color"

static const uint8_t wiper_settings[][3] = {
  {225, 225, 255},
  {51, 255, 51},
  {255, 177, 0}};

uint8_t get_screen_color()
{
  uint8_t color = 0;
  esp_err_t err = nvs_get_u8(storage, KEY_COLOR, &color);
  assert(err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND);
  return color;
}

void set_screen_color(uint8_t color)
{
  for (int i = 0; i < 3; i++) {
    writeDigiPot(i, wiper_settings[color][i]);
  }
  ESP_ERROR_CHECK(nvs_set_u8(storage, KEY_COLOR, color));
  ESP_ERROR_CHECK(nvs_commit(storage));
}
