
#ifndef __SCREEN_H__
#define __SCREEN_H__

#include "z80.h"
#include <stdint.h>


class ScreenBuffer {
private:
  byte*         screenBuffer;
  uint8_t       width;
  uint8_t       height;
  bool          ownBuffer;
  int           currentMode;
  ScreenBuffer* next;

public:
  ScreenBuffer(byte* screenBuffer, uint8_t width, uint8_t height);
  ScreenBuffer(uint8_t width, uint8_t height);
  virtual ~ScreenBuffer();
  void setNext(ScreenBuffer* next);
  ScreenBuffer* getNext();
  void refresh();
  void setExpanded(int flag);
  int isExpandedMode();
  void drawChar(ushort pos, byte character);
};

class Screen {
private:
  ScreenBuffer* top;

public:
  Screen();
  void push(ScreenBuffer* screenBuffer);
  void pop();
  void setExpanded(int flag);
  void drawChar(ushort pos, byte character);
};

extern Screen screen;

#endif
