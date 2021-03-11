
#include "trs_screen.h"
#include "grafyx.h"
#include "fabgl.h"
#include "esp_attr.h"



extern fabgl::Canvas Canvas;


#define MAX_SCALE 1

/* True size of graphics memory -- some is offscreen */
#define G_XSIZE 128
#define G_YSIZE 256

static unsigned char grafyx_unscaled[G_YSIZE][G_XSIZE] EXT_RAM_ATTR;
static unsigned char grafyx_microlabs = 0;
static unsigned char grafyx_x = 0, grafyx_y = 0, grafyx_mode = 0;
static unsigned char grafyx_enable = 0;
static unsigned char grafyx_overlay = 0;
static unsigned char grafyx_xoffset = 0, grafyx_yoffset = 0;

static int row_chars = 64;
static int col_chars = 16;

/* Port 0x83 (grafyx_mode) bits */
#define G_ENABLE    1
#define G_UL_NOTEXT 2   /* Micro-Labs only */
#define G_RS_WAIT   2   /* Radio Shack only */
#define G_XDEC      4
#define G_YDEC      8
#define G_XNOCLKR   16
#define G_YNOCLKR   32
#define G_XNOCLKW   64
#define G_YNOCLKW   128


static void grafyx_write_byte(int x, int y, char byte)
{
  /* Save new byte in local memory */
  assert(x < G_XSIZE && y < G_YSIZE);
  grafyx_unscaled[y][x] = byte;

  if (grafyx_enable) {
    int const screen_x = ((x - grafyx_xoffset + G_XSIZE) % G_XSIZE);
    int const screen_y = ((y - grafyx_yoffset + G_YSIZE) % G_YSIZE);

    /* Draw new byte */
    Canvas.drawGlyph(screen_x * 8, screen_y, 8, 1, (const uint8_t*) &byte);
  }
}


void grafyx_write_x(int value)
{
  grafyx_x = value;
}

void grafyx_write_y(int value)
{
  grafyx_y = value;
}

void grafyx_write_data(int value)
{
  grafyx_write_byte(grafyx_x % G_XSIZE, grafyx_y, value);
  if (!(grafyx_mode & G_XNOCLKW)) {
    if (grafyx_mode & G_XDEC)
      grafyx_x--;
    else
      grafyx_x++;
  }
  if (!(grafyx_mode & G_YNOCLKW)) {
    if (grafyx_mode & G_YDEC)
      grafyx_y--;
    else
      grafyx_y++;
  }
}

int grafyx_read_data(void)
{
  int const value = grafyx_unscaled[grafyx_y][grafyx_x % G_XSIZE];

  if (!(grafyx_mode & G_XNOCLKR)) {
    if (grafyx_mode & G_XDEC)
      grafyx_x--;
    else
      grafyx_x++;
  }
  if (!(grafyx_mode & G_YNOCLKR)) {
    if (grafyx_mode & G_YDEC)
      grafyx_y--;
    else
      grafyx_y++;
  }
  return value;
}

void grafyx_write_mode(int value)
{
  const unsigned char old_enable = grafyx_enable;
  const unsigned char old_overlay = grafyx_overlay;

  grafyx_enable = value & G_ENABLE;
  if (grafyx_microlabs)
    grafyx_overlay = (value & G_UL_NOTEXT) == 0;
  grafyx_mode = value;

  if (old_enable != grafyx_enable) {
    trs_screen.enableGrafyxMode(grafyx_enable);
  }
}
