

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

fabgl::VGA2Controller DisplayController;
fabgl::Canvas         Canvas(&DisplayController);

uint8_t ScreenBuffer::currentMonitorMode = 0;

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
    settingsScreen.init();
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
}

void TRSScreen::init()
{
#ifdef CONFIG_POCKET_TRS_TTGO_VGA32_SUPPORT
  DisplayController.begin(GPIO_NUM_22, GPIO_NUM_21, GPIO_NUM_19, GPIO_NUM_18, GPIO_NUM_5, GPIO_NUM_4, GPIO_NUM_23, GPIO_NUM_15);
#else
  DisplayController.begin(VGA_RED, VGA_GREEN, VGA_BLUE, VGA_HSYNC, VGA_VSYNC);
#endif
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
}

void TRSScreen::pop()
{
  ScreenBuffer* tmp = top;
  top = top->getNext();
  delete tmp;
}

void TRSScreen::setMode(uint8_t mode)
{
  top->setMode(mode);
}

uint8_t TRSScreen::getMode()
{
  return top->getMode();
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
  uint8_t mode = top->getMode();
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
  return (top->getMode() & MODE_GRAFYX) == 0;
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
      trs_printer_write(ch);
    }
    trs_printer_write('\r');
  }
  trs_printer_write('\r');
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

#ifdef CONFIG_POCKET_TRS_TTGO_VGA32_SUPPORT
  switch(color) {
    case SCREEN_COLOR_WHITE:
      DisplayController.setPaletteItem(1, RGB888(0xe0, 0xe0, 0xff));
      break;
    case SCREEN_COLOR_GREEN:
      DisplayController.setPaletteItem(1, RGB888(0x33, 0xff, 0x33));
      break;
    case SCREEN_COLOR_AMBER:
      DisplayController.setPaletteItem(1, RGB888( 0xff, 0xb0, 0x00));
      break;
  }
#else
  for (int i = 0; i < 3; i++) {
    writeDigiPot(i, wiper_settings[color][i]);
  }
#endif
}

SettingsScreen settingsScreen;

