#if 0
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


fabgl::VGA16Controller DisplayController;
fabgl::Canvas        Canvas(&DisplayController);
fabgl::PS2Controller PS2Controller;

void setup() {
  Serial.begin(115200);
  Serial.print("Size PSRAM: ");
  Serial.println(ESP.getPsramSize());
  Serial.print("Heap size before VGA init: ");
  Serial.println(ESP.getFreeHeap());

  init_events();
  init_trs_io();
  init_storage();
  init_trs();
  z80_reset(0);
  init_i2s();
  init_io();
  init_settings();
  //init_wifi();
  //init_time();
  //start_mg(true);
  //return;
  Serial.printf("A\n");
  //delay(5000);
  Serial.printf("B\n");
  DisplayController.begin(VGA_RED, VGA_GREEN, VGA_BLUE, VGA_HSYNC, VGA_VSYNC);
  Serial.printf("C\n");
  DisplayController.setResolution(VGA_640x480_60Hz);//VGA_512x192_60Hz);
  //DisplayController.enableBackgroundPrimitiveExecution(false);
  //DisplayController.enableBackgroundPrimitiveTimeout(false);
  Serial.printf("D\n");
  Canvas.setBrushColor(Color::White);//Black);
  Canvas.setGlyphOptions(GlyphOptions().FillBackground(true));
  Canvas.setPenColor(Color::White);
  Serial.printf("E\n");
  PS2Controller.begin(PS2Preset::KeyboardPort0, KbdMode::CreateVirtualKeysQueue);
  Serial.print("Heap size after VGA init: ");
  Serial.println(ESP.getFreeHeap());
  Serial.print("Free heap: ");
  Serial.println(esp_get_free_heap_size());
  Canvas.clear();
  init_wifi();
  delay(5000);
  Serial.printf("F\n");
  
}

void loop() {
  static fabgl::VirtualKey lastvk = fabgl::VK_NONE;
  auto keyboard = PS2Controller.keyboard();

  z80_run();
  if (keyboard == nullptr) {
	  Serial.printf("nullptr\n");
	  return;
  }
  if (!keyboard->isKeyboardAvailable()) {
	  Serial.printf("Keyboard not available\n");
	  return;
  }
  if (keyboard->virtualKeyAvailable()) {
    bool down;
    auto vk = keyboard->getNextVirtualKey(&down);
    //Serial.printf("VirtualKey = %s\n", keyboard->virtualKeyToString(vk));
    if (down && vk == fabgl::VK_F3) {
      configure_pocket_trs();
    } else if (down && vk == fabgl::VK_F9) {
      z80_reset();
    } else {
      process_key(vk, down);
    }
  }
}
#endif
