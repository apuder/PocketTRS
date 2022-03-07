
#include "trs-lib.h"

static window_t wnd;

void help()
{
  set_screen_to_background();
  init_window(&wnd, 0, 3, 0, 0);
  header("Help");
  wnd_print(&wnd, false, "\nKey mapping:\n");
  wnd_print(&wnd, false, "  <F4>  - Enable lowercase\n");
  wnd_print(&wnd, false, "  <F5>  - PocketTRS Configuration\n");
  wnd_print(&wnd, false, "  <F6>  - Send screenshot to the printer\n");
  wnd_print(&wnd, false, "  <F9>  - Reset Z80\n");
  wnd_print(&wnd, false, "  <ESC> - <BREAK>\n");
  wnd_print(&wnd, false, "  \\    - <CLEAR>\n");
  wnd_print(&wnd, false, "\nVisit https://github.com/apuder/PocketTRS for "
            "complete online documentation.");
  screen_show(false);
  get_key();
}
