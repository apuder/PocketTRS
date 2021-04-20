

#include "trs.h"
#include "trs_screen.h"
#include "fabgl.h"
#include "spi.h"
#include "config.h"
#include "wifi.h"
#include "settings.h"


#define MAX_TRS_SCREEN_WIDTH 80
#define MAX_TRS_SCREEN_HEIGHT 24

#define TRS_M3_CHAR_WIDTH 8
#define TRS_M3_CHAR_HEIGHT 12

#define TRS_M4_CHAR_WIDTH 8
#define TRS_M4_CHAR_HEIGHT 10

#include "font/font_m3"
#include "font/font_m4"

//----------------------------------------------------------------
static const ushort m3toUnicode[] = {
  // Special characters, highly ad-hoc.
  0x20,   0xa3,   0x7c,   0xe9,   0xdc,   0xc5,   0xac,   0xf6,
  0xd8,   0xf9,   0xf1,   0x60, 0x0101, 0xe00d,   0xc4,   0xc3,
  0xd1,   0xd6,   0xd8,   0xd5,   0xdf,   0xfc,   0xf5,   0xe6,
      0xe4,   0xe0, 0x0227, 0xe01b,   0xc9,   0xc6,   0xc7, 0x02dc,

    // ASCII range.  Identity map of 20 .. 7e with special case for 7f.
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
    0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
    0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0xb1,

    // Graphics characters.  Trivial map of 80 .. BF to E080 .. E0BF.
    0xe080, 0xe081, 0xe082, 0xe083, 0xe084, 0xe085, 0xe086, 0xe087,
    0xe088, 0xe089, 0xe08a, 0xe08b, 0xe08c, 0xe08d, 0xe08e, 0xe08f,
    0xe090, 0xe091, 0xe092, 0xe093, 0xe094, 0xe095, 0xe096, 0xe097,
    0xe098, 0xe099, 0xe09a, 0xe09b, 0xe09c, 0xe09d, 0xe09e, 0xe09f,
    0xe0a0, 0xe0a1, 0xe0a2, 0xe0a3, 0xe0a4, 0xe0a5, 0xe0a6, 0xe0a7,
    0xe0a8, 0xe0a9, 0xe0aa, 0xe0ab, 0xe0ac, 0xe0ad, 0xe0ae, 0xe0af,
    0xe0b0, 0xe0b1, 0xe0b2, 0xe0b3, 0xe0b4, 0xe0b5, 0xe0b6, 0xe0b7,
    0xe0b8, 0xe0b9, 0xe0ba, 0xe0bb, 0xe0bc, 0xe0bd, 0xe0be, 0xe0bf,

    // Special characters.  Mostly ad-hoc, but contiguous stretch for
    // the lowercase Greek letters.
    0x2660, 0x2665, 0x2666, 0x2663, 0x263a, 0x2639, 0x2264, 0x2265,
    0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7, 0x03b8,
    0x03b9, 0x03ba, 0x03bc, 0x03bd, 0x03be, 0x03bf, 0x03c0, 0x03c1,
    0x03c2, 0x03c3, 0x03c4, 0x03c5, 0x03c6, 0x03c7, 0x03c8, 0x03c9,
    0x2126, 0x221a,   0xf7, 0x2211, 0x2248, 0x2206, 0x2307, 0x2260,
    0x2301, 0xe0e9, 0x237e, 0x221e, 0x2713,   0xa7, 0x2318,   0xa9,
      0xa4,   0xb6,   0xa2,   0xae, 0xe0f4, 0xe0f5, 0xe0f6, 0x211e,
    0x2105, 0x2642, 0x2640, 0xe0fb, 0xe0fc, 0xe0fd, 0xe0fe, 0x2302,

    // Halfwidth Katakana.  Trivial map of C1 .. FF to FF61 .. FF9F
    // with special case for C0 (Yen sign).
      0xa5, 0xff61, 0xff62, 0xff63, 0xff64, 0xff65, 0xff66, 0xff67,
    0xff68, 0xff69, 0xff6a, 0xff6b, 0xff6c, 0xff6d, 0xff6e, 0xff6f,
    0xff70, 0xff71, 0xff72, 0xff73, 0xff74, 0xff75, 0xff76, 0xff77,
    0xff78, 0xff79, 0xff7a, 0xff7b, 0xff7c, 0xff7d, 0xff7e, 0xff7f,
    0xff80, 0xff81, 0xff82, 0xff83, 0xff84, 0xff85, 0xff86, 0xff87,
    0xff88, 0xff89, 0xff8a, 0xff8b, 0xff8c, 0xff8d, 0xff8e, 0xff8f,
    0xff90, 0xff91, 0xff92, 0xff93, 0xff94, 0xff95, 0xff96, 0xff97,
    0xff98, 0xff99, 0xff9a, 0xff9b, 0xff9c, 0xff9d, 0xff9e, 0xff9f
    };
//----------------------------------------------------------------

fabgl::VGA2Controller DisplayController;
fabgl::Canvas         Canvas(&DisplayController);

uint8_t ScreenBuffer::currentMonitorMode = MODE_TEXT_64x16;

ScreenBuffer::ScreenBuffer(uint8_t mode)
{
  this->screenBuffer = (byte*) malloc(MAX_TRS_SCREEN_WIDTH * MAX_TRS_SCREEN_HEIGHT);
  assert(screenBuffer != NULL);
  this->width = 0;
  this->height = 0;
  this->char_width = 0;
  this->char_height = 0;
  this->font = nullptr;
  this->next = nullptr;
  this->currentMonitorMode = 0;
  this->isPaintingInverse = false;
  setMode(mode);
  clear();
}

ScreenBuffer::~ScreenBuffer()
{
  free(screenBuffer);
}

void ScreenBuffer::setMode(uint8_t mode)
{
  bool hires = false;
  bool changeResolution = false;
  uint8_t changes = mode ^ currentMonitorMode;

  if (mode & MODE_TEXT_64x16) {
    width = 64;
    height = 16;
    screen_chars = 64 * 16;
    char_width = TRS_M3_CHAR_WIDTH;
    char_height = TRS_M3_CHAR_HEIGHT;
    font = (byte*) font_m3;
    if (changes & MODE_TEXT_64x16) {
      changeResolution = true;
    }
  }

  if (mode & MODE_TEXT_80x24) {
    width = 80;
    height = 24;
    screen_chars = 80 * 24;
    char_width = TRS_M4_CHAR_WIDTH;
    char_height = TRS_M4_CHAR_HEIGHT;
    font = (byte*) font_m4;
    if (changes & MODE_TEXT_80x24) {
      changeResolution = true;
      hires = true;
    }
  }

  if (changes & MODE_GRAFYX) {
    if ((mode & MODE_GRAFYX) && (currentMonitorMode & MODE_TEXT_64x16)) {
      // Enable Grafyx but the current resolution is 64x16
      hires = true;
      changeResolution = true;
    }
    if (!(mode & MODE_GRAFYX) && (currentMonitorMode & MODE_TEXT_64x16)) {
      // Disable Grafyx and switch back to low res
      hires = false;
      changeResolution = true;
    }
  }

  if (changeResolution) {
    const char* modline = hires ? VGA_640x240_60Hz : VGA_512x192_60Hz;

    DisplayController.setResolution(modline);
    Canvas.reset();
    Canvas.setBrushColor(Color::Black);
    Canvas.setGlyphOptions(GlyphOptions().FillBackground(true));
    Canvas.setPenColor(Color::White);
    Canvas.clear();
    clear();
  }
  currentMonitorMode &= ~(MODE_TEXT_64x16 | MODE_TEXT_80x24);
  currentMonitorMode |= mode;
}

uint8_t ScreenBuffer::getMode()
{
  return currentMonitorMode;
}

uint8_t ScreenBuffer::getWidth()
{
  return width;
}

uint8_t ScreenBuffer::getHeight()
{
  return height;
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
  assert(buf != nullptr && width == buf->width && height == buf->height);
  memcpy(screenBuffer, buf->screenBuffer, screen_chars);
}

void ScreenBuffer::clear()
{
  memset(screenBuffer, ' ', MAX_TRS_SCREEN_WIDTH * MAX_TRS_SCREEN_HEIGHT);
}

void ScreenBuffer::refresh()
{
  for (int pos = 0; pos < screen_chars; pos++) {
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
  int bit = flag ? MODE_EXPANDED : 0;
  if ((currentMonitorMode ^ bit) & MODE_EXPANDED) {
    currentMonitorMode ^= MODE_EXPANDED;
    Canvas.setGlyphOptions(GlyphOptions().DoubleWidth(flag ? 1 : 0)
                                          .FillBackground(true));
    refresh();
  }
}

void ScreenBuffer::setInverse(int flag)
{
  if (flag) {
    currentMonitorMode |= MODE_INVERSE;
  } else {
    currentMonitorMode &= ~MODE_INVERSE;
  }
}

int ScreenBuffer::isExpandedMode()
{
  return (currentMonitorMode & MODE_EXPANDED) != 0;
}

void ScreenBuffer::drawChar(ushort pos, byte character)
{
  if (pos >= screen_chars) {
    return;
  }
  
  if (isExpandedMode() && (pos & 1) != 0) {
    return;
  }
  screenBuffer[pos] = character;
  int pos_x = (pos % width) * char_width;
  int pos_y = (pos / width) * char_height;
  if ((currentMonitorMode & MODE_INVERSE) && (character & 0x80)) {
    if (!isPaintingInverse) {
      Canvas.setBrushColor(Color::White);
      Canvas.setPenColor(Color::Black);
      isPaintingInverse = true;
    }
    character &= 0x7f;
  } else if (isPaintingInverse) {
    Canvas.setBrushColor(Color::Black);
    Canvas.setPenColor(Color::White);
    isPaintingInverse = false;
  }
  Canvas.drawGlyph(pos_x, pos_y, char_width, char_height,
    font, character);
}

bool ScreenBuffer::getChar(ushort pos, byte& character)
{
  if (pos < screen_chars) {
    character = screenBuffer[pos];
    return true;
  }
  return false;
}

//----------------------------------------------------------------

TRSScreen::TRSScreen()
{
  top = nullptr;
  mode = 0;
}

void TRSScreen::init()
{
  DisplayController.begin(VGA_RED, VGA_GREEN, VGA_BLUE, VGA_HSYNC, VGA_VSYNC);
  DisplayController.setResolution(VGA_512x192_60Hz);
  DisplayController.enableBackgroundPrimitiveExecution(false);
  DisplayController.enableBackgroundPrimitiveTimeout(false);

  Canvas.setBrushColor(Color::Black);
  Canvas.setGlyphOptions(GlyphOptions().FillBackground(true));
  Canvas.setPenColor(Color::White);
}

void TRSScreen::push(ScreenBuffer* screenBuffer)
{
  screenBuffer->setNext(top);
  top = screenBuffer;
  mode = top->getMode();
}

void TRSScreen::pop()
{
  ScreenBuffer* tmp = top;
  top = top->getNext();
  mode = (top == nullptr) ? 0 : top->getMode();
  delete tmp;
}

void TRSScreen::setMode(uint8_t mode)
{
  top->setMode(mode);
  this->mode = top->getMode();
}

uint8_t TRSScreen::getMode()
{
  return mode;
}

uint8_t TRSScreen::getWidth()
{
  return top->getWidth();
}

uint8_t TRSScreen::getHeight()
{
  return top->getHeight();
}

void TRSScreen::enableGrafyxMode(bool enable)
{
  if (enable) {
    mode |= MODE_GRAFYX;
  } else {
    mode &= ~MODE_GRAFYX;
  }
  top->setMode(mode);
}

void TRSScreen::setExpanded(int flag)
{
  assert(top != nullptr);
  top->setExpanded(flag);
}

void TRSScreen::setInverse(int flag)
{
  assert(top != nullptr);
  top->setInverse(flag);
}

bool TRSScreen::isTextMode()
{
  return (mode & MODE_GRAFYX) == 0;
}

void TRSScreen::drawChar(ushort pos, byte character)
{
  assert(top != nullptr);
  //if (isTextMode()) {
    top->drawChar(pos, character);
  //}
}

bool TRSScreen::getChar(ushort pos, byte& character)
{
  assert(top != nullptr);
  return top->getChar(pos, character);
}

void TRSScreen::clear()
{
  assert(top != nullptr);
  top->clear();
}

void TRSScreen::refresh()
{
  assert(top != nullptr);
  top->refresh();
}

void TRSScreen::screenshot()
{
  if (top == nullptr || !isTextMode() || trs_printer_read() == 0xff) {
    return;
  }
  ushort pos = 0;
  for (int y = 0; y < getHeight(); y++) {
    for (int x = 0; x < getWidth(); x++) {
      byte ch;
      getChar(pos++, ch);
      trs_printer_write(m3toUnicode[ch] & 0xff);
      trs_printer_write(m3toUnicode[ch] >> 8);
      #if 0
      byte utf8 = m3toUnicode[ch] >> 8;
      if (utf8 != 0) {
        trs_printer_write(utf8 & 0xff);
      }
      trs_printer_write(m3toUnicode[ch] & 0xff);
      #endif
    }
    trs_printer_write('\n');
  }
  trs_printer_write('\n');
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

