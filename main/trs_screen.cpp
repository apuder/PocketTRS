

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

#include "font/font-data"

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
    Canvas.setGlyphOptions(GlyphOptions().DoubleWidth(flag ? 1 : 0)
                                         .FillBackground(true));
    refresh();
  }
}

int ScreenBuffer::isExpandedMode()
{
  return (currentMode & EXPANDED) != 0;
}

void ScreenBuffer::drawChar(ushort pos, byte character)
{
  if (isExpandedMode() && (pos & 1) != 0) {
    return;
  }
  int pos_x = (pos % width) * TRS_CHAR_WIDTH;
  int pos_y = (pos / width) * TRS_CHAR_HEIGHT;
  Canvas.drawGlyph(pos_x, pos_y, TRS_CHAR_WIDTH, TRS_CHAR_HEIGHT,
    font_single, character);
}


//----------------------------------------------------------------

TRSScreen::TRSScreen()
{
  top = nullptr;
  text_mode = true;
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
  if (text_mode) {
    top->setExpanded(flag);
  }
}

void TRSScreen::setTextMode(bool flag)
{
  text_mode = flag;
  if (text_mode) {
    top->refresh();
  }
}

bool TRSScreen::isTextMode()
{
  return text_mode;
}

void TRSScreen::drawChar(ushort pos, byte character)
{
  assert(top != nullptr);
  if (text_mode) {
    top->drawChar(pos, character);
  }
}

void TRSScreen::refresh()
{
  assert(top != nullptr);
  if (text_mode) {
    top->refresh();
  }
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

