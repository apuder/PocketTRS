
#include "fabgl.h"
#include "trs.h"
#include "trs-keyboard.h"
#include "sound.h"
#include "cassette.h"
#include "io.h"
#include "config.h"

void setup() {
  Serial.begin(115200);
  Serial.print("Heap size before VGA init: ");
  Serial.println(ESP.getFreeHeap());
  PS2Controller.begin(PS2Preset::KeyboardPort0);
  VGAController.begin(VGA_RED, VGA_GREEN, VGA_BLUE, VGA_HSYNC, VGA_VSYNC);
  VGAController.setResolution(VGA_512x192_60Hz);
  VGAController.enableBackgroundPrimitiveExecution(false);
  VGAController.enableBackgroundPrimitiveTimeout(false);
  Canvas.setBrushColor(Color::Black);
  Canvas.setGlyphOptions(GlyphOptions().FillBackground(true));
  Canvas.setPenColor(Color::White);
  Serial.print("Heap size after VGA init: ");
  Serial.println(ESP.getFreeHeap());
  Canvas.clear();
  init_sound();
  Serial.println("Ready");
  z80_reset(0);
  // init_cassette_in();
  init_io();
  Serial.println("Done");
}

void loop() {
  #if 0
  byte b = 0;
  while (true) {
    //ground latchPin and hold low for as long as you are transmitting
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, b);
    shiftOut(dataPin, clockPin, MSBFIRST, b);
    //return the latch pin high to signal chip that it
    //no longer needs to listen for information
    digitalWrite(latchPin, HIGH);
    delay(500);
    b ^= 0xff;
  }
  #endif
  static fabgl::VirtualKey lastvk = fabgl::VK_NONE;

  z80_run();
  if (Keyboard.virtualKeyAvailable()) {
    bool down;
    auto vk = Keyboard.getNextVirtualKey(&down);
    //if (vk != lastvk) {
      process_key(vk, down);
    //  lastvk = down ? vk : fabgl::VK_NONE;
    //}
  }
}
