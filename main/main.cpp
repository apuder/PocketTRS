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
#include "ota.h"
#include "storage.h"
#include "event.h"
#include "freertos/task.h"

#include "trs-io.h"
#include "ntp_sync.h"


fabgl::PS2Controller  PS2Controller;


void setup() {
#if 1
  printf("Heap size before VGA init: %d\n", esp_get_free_heap_size());
  printf("DRAM size before VGA init: %d\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
#endif

  init_button();
  if (is_button_pressed()) {
    // Run hardware tests
  }
  init_events();
  init_trs_io();
  init_storage();
  init_i2s();
  init_io();
  init_settings();
  trs_screen.init();
  show_splash();
  init_wifi();
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  settingsCalibration.setScreenOffset();
  PS2Controller.begin(PS2Preset::KeyboardPort0, KbdMode::CreateVirtualKeysQueue);

  z80_reset(0);

#if 1
  printf("Heap size after VGA init: %d\n", esp_get_free_heap_size());
  printf("DRAM size after VGA init: %d\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
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
    if (down && vk == fabgl::VK_F3 && trs_screen.isTextMode()) {
      configure_pocket_trs();
    } else if (down && vk == fabgl::VK_F9) {
      z80_reset();
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
