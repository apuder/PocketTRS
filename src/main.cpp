
#include "fabgl.h"
#include "trs.h"
#include "trs-keyboard.h"
#include "sound.h"


//Pin connected to ST_CP of 74HC595
int latchPin = 27;
//Pin connected to SH_CP of 74HC595
int clockPin = 4;
////Pin connected to DS of 74HC595
int dataPin = 2;

void setup() {
  Serial.begin(115200);
  Serial.print("Heap size before VGA init: ");
  Serial.println(ESP.getFreeHeap());
  PS2Controller.begin(PS2Preset::KeyboardPort0);
  VGAController.begin(GPIO_NUM_22, GPIO_NUM_21, GPIO_NUM_23, GPIO_NUM_26, GPIO_NUM_5);
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
  z80_reset(0);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
}

void loop() {
  #if 0
  for (int j = 0; j < 256; j++) {
    //ground latchPin and hold low for as long as you are transmitting
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, j);
    //return the latch pin high to signal chip that it
    //no longer needs to listen for information
    digitalWrite(latchPin, HIGH);
    delay(1000);
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
