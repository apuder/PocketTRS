
#ifndef __TRS_SCREEN_H__
#define __TRS_SCREEN_H__

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
  uint8_t* getBuffer();
  void setNext(ScreenBuffer* next);
  ScreenBuffer* getNext();
  void copyBufferFrom(ScreenBuffer* buf);
  void refresh();
  void update(uint8_t* from, uint8_t* to);
  void setExpanded(int flag);
  int isExpandedMode();
  void drawChar(ushort pos, byte character);
};

class TRSScreen {
private:
  ScreenBuffer* top;

public:
  TRSScreen();
  void push(ScreenBuffer* screenBuffer);
  void pop();
  void setExpanded(int flag);
  void drawChar(ushort pos, byte character);
};

extern TRSScreen trs_screen;

#endif
