

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
static const char* m3toUnicode[] = {
  u8"\u0020", u8"\u00a3", u8"\u007c", u8"\u00e9", u8"\u00dc", u8"\u00c5", u8"\u00ac", u8"\u00f6", 
  u8"\u00d8", u8"\u00f9", u8"\u00f1", u8"\u0060", u8"\u0101", u8"\ue00d", u8"\u00c4", u8"\u00c3", 
  u8"\u00d1", u8"\u00d6", u8"\u00d8", u8"\u00d5", u8"\u00df", u8"\u00fc", u8"\u00f5", u8"\u00e6", 
  u8"\u00e4", u8"\u00e0", u8"\u0227", u8"\ue01b", u8"\u00c9", u8"\u00c6", u8"\u00c7", u8"\u02dc", 
  u8"\u0020", u8"\u0021", u8"\u0022", u8"\u0023", u8"\u0024", u8"\u0025", u8"\u0026", u8"\u0027", 
  u8"\u0028", u8"\u0029", u8"\u002a", u8"\u002b", u8"\u002c", u8"\u002d", u8"\u002e", u8"\u002f", 
  u8"\u0030", u8"\u0031", u8"\u0032", u8"\u0033", u8"\u0034", u8"\u0035", u8"\u0036", u8"\u0037", 
  u8"\u0038", u8"\u0039", u8"\u003a", u8"\u003b", u8"\u003c", u8"\u003d", u8"\u003e", u8"\u003f", 
  u8"\u0040", u8"\u0041", u8"\u0042", u8"\u0043", u8"\u0044", u8"\u0045", u8"\u0046", u8"\u0047", 
  u8"\u0048", u8"\u0049", u8"\u004a", u8"\u004b", u8"\u004c", u8"\u004d", u8"\u004e", u8"\u004f", 
  u8"\u0050", u8"\u0051", u8"\u0052", u8"\u0053", u8"\u0054", u8"\u0055", u8"\u0056", u8"\u0057", 
  u8"\u0058", u8"\u0059", u8"\u005a", u8"\u005b", u8"\u005c", u8"\u005d", u8"\u005e", u8"\u005f", 
  u8"\u0060", u8"\u0061", u8"\u0062", u8"\u0063", u8"\u0064", u8"\u0065", u8"\u0066", u8"\u0067", 
  u8"\u0068", u8"\u0069", u8"\u006a", u8"\u006b", u8"\u006c", u8"\u006d", u8"\u006e", u8"\u006f", 
  u8"\u0070", u8"\u0071", u8"\u0072", u8"\u0073", u8"\u0074", u8"\u0075", u8"\u0076", u8"\u0077", 
  u8"\u0078", u8"\u0079", u8"\u007a", u8"\u007b", u8"\u007c", u8"\u007d", u8"\u007e", u8"\u00b1", 
  u8"\ue080", u8"\ue081", u8"\ue082", u8"\ue083", u8"\ue084", u8"\ue085", u8"\ue086", u8"\ue087", 
  u8"\ue088", u8"\ue089", u8"\ue08a", u8"\ue08b", u8"\ue08c", u8"\ue08d", u8"\ue08e", u8"\ue08f", 
  u8"\ue090", u8"\ue091", u8"\ue092", u8"\ue093", u8"\ue094", u8"\ue095", u8"\ue096", u8"\ue097", 
  u8"\ue098", u8"\ue099", u8"\ue09a", u8"\ue09b", u8"\ue09c", u8"\ue09d", u8"\ue09e", u8"\ue09f", 
  u8"\ue0a0", u8"\ue0a1", u8"\ue0a2", u8"\ue0a3", u8"\ue0a4", u8"\ue0a5", u8"\ue0a6", u8"\ue0a7", 
  u8"\ue0a8", u8"\ue0a9", u8"\ue0aa", u8"\ue0ab", u8"\ue0ac", u8"\ue0ad", u8"\ue0ae", u8"\ue0af", 
  u8"\ue0b0", u8"\ue0b1", u8"\ue0b2", u8"\ue0b3", u8"\ue0b4", u8"\ue0b5", u8"\ue0b6", u8"\ue0b7", 
  u8"\ue0b8", u8"\ue0b9", u8"\ue0ba", u8"\ue0bb", u8"\ue0bc", u8"\ue0bd", u8"\ue0be", u8"\ue0bf", 
  u8"\u2660", u8"\u2665", u8"\u2666", u8"\u2663", u8"\u263a", u8"\u2639", u8"\u2264", u8"\u2265", 
  u8"\u03b1", u8"\u03b2", u8"\u03b3", u8"\u03b4", u8"\u03b5", u8"\u03b6", u8"\u03b7", u8"\u03b8", 
  u8"\u03b9", u8"\u03ba", u8"\u03bc", u8"\u03bd", u8"\u03be", u8"\u03bf", u8"\u03c0", u8"\u03c1", 
  u8"\u03c2", u8"\u03c3", u8"\u03c4", u8"\u03c5", u8"\u03c6", u8"\u03c7", u8"\u03c8", u8"\u03c9", 
  u8"\u2126", u8"\u221a", u8"\u00f7", u8"\u2211", u8"\u2248", u8"\u2206", u8"\u2307", u8"\u2260", 
  u8"\u2301", u8"\ue0e9", u8"\u237e", u8"\u221e", u8"\u2713", u8"\u00a7", u8"\u2318", u8"\u00a9", 
  u8"\u00a4", u8"\u00b6", u8"\u00a2", u8"\u00ae", u8"\ue0f4", u8"\ue0f5", u8"\ue0f6", u8"\u211e", 
  u8"\u2105", u8"\u2642", u8"\u2640", u8"\ue0fb", u8"\ue0fc", u8"\ue0fd", u8"\ue0fe", u8"\u2302"
};

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
      const char* uni = m3toUnicode[ch];
      trs_printer_write(uni);
    }
    trs_printer_write("\n");
  }
  trs_printer_write("\n");
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

