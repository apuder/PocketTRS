
#include "trs-lib.h"

static window_t wnd;

void status()
{
  set_screen_to_background();
  init_window(&wnd, 0, 3, 0, 0);
  header("Status");
  wnd_print(&wnd, false, "\nTBD");
  screen_show(false);
  get_key();
}
