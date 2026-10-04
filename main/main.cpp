#include "fabgl.h"
#include "trs.h"
#include "trs-keyboard.h"
#include "trs_screen.h"
#include "i2s.h"
#include "cassette.h"
#include "io.h"
#include "ui.h"
#include "settings.h"
#include "splash.h"

#include "button.h"
#include "led.h"
#include "wifi.h"
#include "event.h"
#include "trs_io_host.h"
#include "freertos/task.h"

#include "trs-io.h"
#include "trs-fs.h"
#include "ntp_sync.h"


fabgl::PS2Controller  PS2Controller;

// The phosphor color picked in TRS-IO's web UI. Called from the web server's
// task; TRS-IO has already stored its copy.
static void apply_web_screen_color(uint8_t color)
{
  if (color <= SCREEN_COLOR_AMBER) {
    settingsScreen.setScreenColor((screen_color_t) color);
  }
}

// What TRS-IO's own main loop does besides serving the Z80: once Wi-Fi is
// up, mount the SMB share and start the web server.
static void trs_io_task(void* arg)
{
  while (true) {
    trs_io_host_poll();
    vTaskDelay(250 / portTICK_PERIOD_MS);
  }
}


void setup() {
#if 1
  printf("Heap size before VGA init: %u\n", (unsigned) esp_get_free_heap_size());
  printf("DRAM size before VGA init: %u\n", (unsigned) heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
#endif

  init_button();
  init_events();
  init_trs_io();
  // TRS-IO's settings (Wi-Fi, SMB share, time zone; NVS "retrostore")
  trs_io_host_init();
  init_io();
  init_i2s();
  trs_screen.init();
  // Also gives TRS-IO's web UI the current phosphor color
  init_settings();
  trs_io_host_set_screen_color_handler(apply_web_screen_color);
  show_splash();
  init_trs_fs_posix();
  // Wi-Fi, the web UI and, from trs_io_task, the SMB share
  trs_io_host_start_network();
  xTaskCreatePinnedToCore(trs_io_task, "trs-io", 6000, NULL, 1, NULL, 1);
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  //settingsCalibration.setScreenOffset();
  PS2Controller.begin(PS2Preset::KeyboardPort0, KbdMode::CreateVirtualKeysQueue);

  z80_reset(0);

#if 1
  printf("Heap size after VGA init: %u\n", (unsigned) esp_get_free_heap_size());
  printf("DRAM size after VGA init: %u\n", (unsigned) heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
#endif
}

void loop() {
  static fabgl::VirtualKey lastvk = fabgl::VK_NONE;
  auto keyboard = PS2Controller.keyboard();

  z80_run();

  if (is_button_short_press()) {
    z80_reset();
  }

  if (keyboard == nullptr || !keyboard->isKeyboardAvailable()) {
    return;
  }
  if (keyboard->virtualKeyAvailable()) {
    bool down;
    auto vk = keyboard->getNextVirtualKey(&down);
    //printf("VirtualKey = %s\n", keyboard->virtualKeyToString(vk));
    if (down && vk == fabgl::VK_F5 && trs_screen.isTextMode()) {
      configure_pocket_trs();
    } else if (down && vk == fabgl::VK_F9) {
      z80_reset();
    } else if (down && vk == fabgl::VK_F6) {
      trs_screen.screenshot();
    } else {
      process_key(vk, down);
    }
  }
}

extern "C" void app_main()
{
  setup();
  while (true) loop();
}
