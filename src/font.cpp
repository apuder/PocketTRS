

#include "trs.h"
#include "font.h"
#include "fabgl.h"

#define NORMAL 0
#define EXPANDED 1
#define INVERSE 2
#define ALTERNATE 4

#define TRS_CHAR_WIDTH 8
#define TRS_CHAR_HEIGHT 12

#include "font/font-data-single"
#include "font/font-data-double"

static int currentmode = NORMAL;

static void trs_screen_refresh()
{
  for (int pos = 0; pos < 64 * 16; pos++) {
    ushort address = 0x3c00 + pos;
    draw_trs_char(address, peek_mem(address));
  }
}

void trs_screen_expanded(int flag)
{
  int bit = flag ? EXPANDED : 0;
  if ((currentmode ^ bit) & EXPANDED) {
    currentmode ^= EXPANDED;
    Canvas.setGlyphOptions(GlyphOptions().DoubleWidth(((currentmode & EXPANDED) == 0) ? 0 : 1));
    trs_screen_refresh();
  }
}

static inline int is_expanded_mode()
{
  return currentmode & EXPANDED;
}

void draw_trs_char(ushort address, byte character)
{
  if (is_expanded_mode() && (address & 1) != 0) {
    return;
  }
  int d = is_expanded_mode() ? 2 : 1;
  address -= 0x3c00;
  int pos_x = (address % 64) * TRS_CHAR_WIDTH * d;
  int pos_y = (address / 64) * TRS_CHAR_HEIGHT;
//  Canvas.drawGlyph(pos_x, pos_y, TRS_CHAR_WIDTH * d, TRS_CHAR_HEIGHT,
//    is_expanded_mode() ? font_double : font_single, character);
  Canvas.drawGlyph(pos_x, pos_y, TRS_CHAR_WIDTH * d, TRS_CHAR_HEIGHT,
    is_expanded_mode() ? font_double : font_single, character);
}
