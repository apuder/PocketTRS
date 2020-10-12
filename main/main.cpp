#include "fabgl.h"
#include "trs.h"
#include "trs-keyboard.h"
#include "i2s.h"
#include "cassette.h"
#include "io.h"
#include "ui.h"
#include "settings.h"
#include "config.h"

#include "led.h"
#include "wifi.h"
#include "ota.h"
#include "storage.h"
#include "event.h"
#include "freertos/task.h"

#include "trs-io.h"
#include "ntp_sync.h"


fabgl::VGA2Controller DisplayController;
fabgl::Canvas        Canvas(&DisplayController);
fabgl::PS2Controller PS2Controller;

void setup() {
#if 0
  printf("Size PSRAM: %d\n", ESP.getPsramSize());
  printf("Heap size before VGA init: %d\n", ESP.getFreeHeap());
#endif

  init_events();
  init_trs_io();
  init_storage();
  init_trs();
  z80_reset(0);
  init_i2s();
  init_io();
  init_settings();
  init_wifi();
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  DisplayController.begin(VGA_RED, VGA_GREEN, VGA_BLUE, VGA_HSYNC, VGA_VSYNC);
  DisplayController.setResolution(VGA_512x192_60Hz);
  DisplayController.enableBackgroundPrimitiveExecution(false);
  DisplayController.enableBackgroundPrimitiveTimeout(false);
  DisplayController.moveScreen(-5, 0);
  Canvas.setBrushColor(Color::Black);
  Canvas.setGlyphOptions(GlyphOptions().FillBackground(true));
  Canvas.setPenColor(Color::White);
  PS2Controller.begin(PS2Preset::KeyboardPort0, KbdMode::CreateVirtualKeysQueue);

#if 0
  printf("Heap size after VGA init: %d\n", ESP.getFreeHeap());
  printf("Free heap: %d\n", esp_get_free_heap_size());
#endif
  Canvas.clear();
}

void loop() {
  static fabgl::VirtualKey lastvk = fabgl::VK_NONE;
  auto keyboard = PS2Controller.keyboard();

  z80_run();
  if (keyboard == nullptr || !keyboard->isKeyboardAvailable()) {
    return;
  }
  if (keyboard->virtualKeyAvailable()) {
    bool down;
    auto vk = keyboard->getNextVirtualKey(&down);
    //printf("VirtualKey = %s\n", keyboard->virtualKeyToString(vk));
    if (down && vk == fabgl::VK_F3) {
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
