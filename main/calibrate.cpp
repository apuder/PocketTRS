
#include "calibrate.h"
#include "settings.h"
#include "fabgl.h"

extern "C" {
#include "ui.h"
#include "trs-lib.h"
}

#define KEY_CALIBRATION_SCREEN_OFFSET_X "screen_offset_x"
#define KEY_CALIBRATION_SCREEN_OFFSET_Y "screen_offset_y"

extern fabgl::VGA2Controller DisplayController;
extern fabgl::PS2Controller PS2Controller;


void SettingsCalibration::init()
{
  screenOffsetX = nvs_get_i8(KEY_CALIBRATION_SCREEN_OFFSET_X);
  screenOffsetY = nvs_get_i8(KEY_CALIBRATION_SCREEN_OFFSET_Y);
}

void SettingsCalibration::setScreenOffset()
{
  DisplayController.moveScreen(screenOffsetX, screenOffsetY);
}

void SettingsCalibration::moveScreenOffset(int8_t dx, int8_t dy)
{
  screenOffsetX += dx;
  screenOffsetY += dy;
  DisplayController.moveScreen(dx, dy);
}

void SettingsCalibration::saveScreenOffset()
{
  nvs_set_i8(KEY_CALIBRATION_SCREEN_OFFSET_X, screenOffsetX);
  nvs_set_i8(KEY_CALIBRATION_SCREEN_OFFSET_Y, screenOffsetY);
}

SettingsCalibration settingsCalibration;


static char get_next_key(bool& is_shifted)
{
  auto keyboard = PS2Controller.keyboard();

  is_shifted = false;
  while (true) {
    vTaskDelay(portTICK_PERIOD_MS);
    if (!keyboard->virtualKeyAvailable()) {
      continue;
    }
  
    bool down;
    auto vk = keyboard->getNextVirtualKey(&down);
    if (!down) {
      is_shifted = false;
      continue;
    }
    switch(vk) {
    case fabgl::VK_LSHIFT:
    case fabgl::VK_RSHIFT:
      is_shifted = true;
      continue;
    case fabgl::VK_ESCAPE:
      return KEY_BREAK;
    case fabgl::VK_UP:
    case fabgl::VK_KP_UP:
      return KEY_UP;
    case fabgl::VK_DOWN:
    case fabgl::VK_KP_DOWN:
      return KEY_DOWN;
    case fabgl::VK_RIGHT:
    case fabgl::VK_KP_RIGHT:
      return KEY_RIGHT;
    case fabgl::VK_LEFT:
    case fabgl::VK_KP_LEFT:
      return KEY_LEFT;
    default:
      // Ignore
      break;
    }
  }
}

static void draw_calibration_screen()
{
  static window_t wnd;

  set_screen_to_background();
  init_window(&wnd, 0, 0, 0, 0);
  wnd_cls(&wnd);
  wnd_print(&wnd, false, "*");
  for (int x = 0; x < 62; x++) {
    wnd_print(&wnd, false, "\203");
  }
  wnd_print(&wnd, false, "*");
  for (int y = 1; y < 15; y++) {
    wnd_goto(&wnd, 0, y);
    wnd_print(&wnd, false, "\277");
    wnd_goto(&wnd, 63, y);
    wnd_print(&wnd, false, "\277");
  }
  wnd_print(&wnd, false, "*");
  for (int x = 0; x < 62; x++) {
    wnd_print(&wnd, false, "\260");
  }
  //XXX will print "..." because of the way trs-lib's window
  // system handles overlong lines
  wnd_print(&wnd, false, "*");

  int row = 6;
  static const char* msg1 = "Use arrow keys to center the screen.";
  wnd_goto(&wnd, (64 - strlen(msg1)) / 2, row++);
  wnd_print(&wnd, false, msg1);
#if 0
  static const char* msg2 = "Use <SHIFT> & arrow keys to resize the screen.";
  wnd_goto(&wnd, (64 - strlen(msg2)) / 2, row++);
  wnd_print(&wnd, false, msg2);
#endif
  static const char* msg3 = "Hit <ESC> or <ENTER> when done.";
  wnd_goto(&wnd, (64 - strlen(msg3)) / 2, row++);
  wnd_print(&wnd, false, msg3);

  screen_show(false);
}

void calibrate()
{
  draw_calibration_screen();

  while (true) {
    int dx = 0;
    int dy = 0;

    bool is_shifted = false;
    switch(get_next_key(is_shifted)) {
    case KEY_BREAK:
    case KEY_ENTER:
      settingsCalibration.saveScreenOffset();
      return;
    case KEY_LEFT:
      dx = -1;
      break;
    case KEY_RIGHT:
      dx = 1;
      break;
    case KEY_UP:
      dy = -1;
      break;
    case KEY_DOWN:
      dy = 1;
      break;
    default:
      continue;
    }
#if 1
    settingsCalibration.moveScreenOffset(dx, dy);
#else
    if (is_shifted) {
      DisplayController.shrinkScreen(dx, dy);
    } else {
      DisplayController.moveScreen(dx, dy);
    }
#endif
  }
}
