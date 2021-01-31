

#include "trs.h"
#include "trs_screen.h"
#include "fabgl.h"
#include "spi.h"
#include "settings.h"

#define NORMAL 0
#define EXPANDED 1
#define INVERSE 2
#define ALTERNATE 4

#define TRS_CHAR_WIDTH 8
#define TRS_CHAR_HEIGHT 12

#include "font/font-data-single"
#include "font/font-data-double"

//----------------------------------------------------------------

extern fabgl::Canvas Canvas;

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
  assert(screenBuffer != NULL);
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

uint8_t* ScreenBuffer::getBuffer()
{
  return screenBuffer;
}

void ScreenBuffer::setNext(ScreenBuffer* next)
{
  this->next = next;
}

ScreenBuffer* ScreenBuffer::getNext()
{
  return next;
}

void ScreenBuffer::copyBufferFrom(ScreenBuffer* buf)
{
  assert(next != nullptr && width == buf->width && height == buf->height);
  memcpy(screenBuffer, buf->screenBuffer, width * height);
}

void ScreenBuffer::refresh()
{
  for (int pos = 0; pos < width * height; pos++) {
    drawChar(pos, screenBuffer[pos]);
  }
}

void ScreenBuffer::update(uint8_t* from, uint8_t* to)
{
  int pos = from - screenBuffer;
  while (from != to) {
    drawChar(pos, *from);
    pos++;
    from++;
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


//----------------------------------------------------------------

TRSScreen::TRSScreen()
{
  top = nullptr;
}

void TRSScreen::push(ScreenBuffer* screenBuffer)
{
  screenBuffer->setNext(top);
  top = screenBuffer;
}

void TRSScreen::pop()
{
  ScreenBuffer* tmp = top;
  top = top->getNext();
  delete tmp;
}

void TRSScreen::setExpanded(int flag)
{
  assert(top != nullptr);
  top->setExpanded(flag);
}

void TRSScreen::drawChar(ushort pos, byte character)
{
  assert(top != nullptr);
  top->drawChar(pos, character);
}

TRSScreen trs_screen;



//----------------------------------------------------------------

static const char* KEY_COLOR  = "color";


static const uint8_t wiper_settings[][3] = {
  {225, 225, 255},
  {51, 255, 51},
  {255, 177, 0}};

void SettingsScreen::init() {
  setScreenColor(getScreenColor());
}

screen_color_t SettingsScreen::getScreenColor() {
  return (screen_color_t) nvs_get_u8(KEY_COLOR);
}

void SettingsScreen::setScreenColor(screen_color_t color) {
  nvs_set_u8(KEY_COLOR, color);
  for (int i = 0; i < 3; i++) {
    writeDigiPot(i, wiper_settings[color][i]);
  }
}

SettingsScreen settingsScreen;

