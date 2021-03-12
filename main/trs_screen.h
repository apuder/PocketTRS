
#ifndef __TRS_SCREEN_H__
#define __TRS_SCREEN_H__

#include "z80.h"
#include <stdint.h>

#define MODE_NORMAL     (1 << 0)
#define MODE_EXPANDED   (1 << 1)
#define MODE_INVERSE    (1 << 2)
#define MODE_ALTERNATE  (1 << 3)
#define MODE_TEXT_64x16 (1 << 4)
#define MODE_TEXT_80x24 (1 << 5)
#define MODE_GRAFYX     (1 << 6)
#define MODE_TEXT       (MODE_TEXT_64x16 | MODE_TEXT_80x24)

class ScreenBuffer {
private:
  static uint8_t currentMonitorMode;
  
  byte*         screenBuffer;
  uint8_t       width;
  uint8_t       height;
  ushort        screen_chars;
  uint8_t       char_width;
  uint8_t       char_height;
  byte*         font;
  ScreenBuffer* next;

public:
  ScreenBuffer(uint8_t mode);
  virtual ~ScreenBuffer();
  void setMode(uint8_t mode);
  uint8_t getMode();
  uint8_t* getBuffer();
  void setNext(ScreenBuffer* next);
  ScreenBuffer* getNext();
  void copyBufferFrom(ScreenBuffer* buf);
  void clear();
  void refresh();
  void update(uint8_t* from, uint8_t* to);
  void setExpanded(int flag);
  int isExpandedMode();
  void drawChar(ushort pos, byte character);
  byte getChar(ushort pos);
};

class TRSScreen {
private:
  ScreenBuffer* top;
  uint8_t mode;

public:
  TRSScreen();
  void push(ScreenBuffer* screenBuffer);
  void pop();
  void setMode(uint8_t mode);
  void enableGrafyxMode(bool enable);
  void setExpanded(int flag);
  bool isTextMode();
  void drawChar(ushort pos, byte character);
  byte getChar(ushort pos);
  void clear();
  void refresh();
};

extern TRSScreen trs_screen;

#endif
