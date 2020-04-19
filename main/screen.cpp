

#include "trs.h"
#include "screen.h"
#include "fabgl.h"

#define NORMAL 0
#define EXPANDED 1
#define INVERSE 2
#define ALTERNATE 4

#define TRS_CHAR_WIDTH 8
#define TRS_CHAR_HEIGHT 12

#include "font/font-data-single"
#include "font/font-data-double"


ScreenBuffer::ScreenBuffer(byte*   screenBuffer,
			   uint8_t width,
			   uint8_t height)
{
  this->screenBuffer = screenBuffer;
  this->ownBuffer = false;
  this->width = width;
  this->height = height;
  this->currentMode = NORMAL;
  this->next = nullptr;
}

ScreenBuffer::ScreenBuffer(uint8_t width, uint8_t height)
{
  this->screenBuffer = (byte*) malloc(width * height);
  this->ownBuffer = true;
  this->width = width;
  this->height = height;
  this->currentMode = NORMAL;
  this->next = nullptr;
}

ScreenBuffer::~ScreenBuffer()
{
  if (ownBuffer) {
    free(screenBuffer);
  }
}

void ScreenBuffer::setNext(ScreenBuffer* next)
{
  this->next = next;
}

void ScreenBuffer::refresh()
{
  for (int pos = 0; pos < width * height; pos++) {
    drawChar(pos, screenBuffer[pos]);
  }
}

void ScreenBuffer::setExpanded(int flag)
{
  int bit = flag ? EXPANDED : 0;
  if ((currentMode ^ bit) & EXPANDED) {
    currentMode ^= EXPANDED;
    Canvas.setGlyphOptions(GlyphOptions().DoubleWidth(((currentMode & EXPANDED) == 0) ? 0 : 1));
    refresh();
  }
}

int ScreenBuffer::isExpandedMode()
{
  return currentMode & EXPANDED;
}

void ScreenBuffer::drawChar(ushort pos, byte character)
{
  if (isExpandedMode() && (pos & 1) != 0) {
    return;
  }
  int d = isExpandedMode() ? 2 : 1;
  int pos_x = (pos % width) * TRS_CHAR_WIDTH * d;
  int pos_y = (pos / width) * TRS_CHAR_HEIGHT;
//  Canvas.drawGlyph(pos_x, pos_y, TRS_CHAR_WIDTH * d, TRS_CHAR_HEIGHT,
//    is_expanded_mode() ? font_double : font_single, character);
  Canvas.drawGlyph(pos_x, pos_y, TRS_CHAR_WIDTH * d, TRS_CHAR_HEIGHT,
		   isExpandedMode() ? font_double : font_single, character);
}


  

Screen::Screen()
{
  top = nullptr;
}

void Screen::push(ScreenBuffer* screenBuffer)
{
  screenBuffer->setNext(top);
  top = screenBuffer;
}

void Screen::pop()
{
  ScreenBuffer* tmp = top;
  top = top->getNext();
  delete tmp;
}

void Screen::setExpanded(int flag)
{
  assert(top != nullptr);
  top->setExpanded(flag);
}

void Screen::drawChar(ushort pos, byte character)
{
  assert(top != nullptr);
  top->drawChar(pos, character);
}

Screen screen;
