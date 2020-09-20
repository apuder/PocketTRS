
#include "ui.h"
#include "storage.h"
#include "trs_screen.h"
#include "fabgl.h"

extern "C" {
  #include <trs-lib.h>
  #include "settings.h"
}

#define MENU_CONFIGURE 0
#define MENU_STATUS 1
#define MENU_RESET 2
#define MENU_HELP 3
#define MENU_EXIT 4

static menu_item_t main_menu_items[] = {
  {MENU_CONFIGURE, "Configure"},
  {MENU_STATUS, "Status"},
  {MENU_RESET, "Reset Settings"},
  {MENU_HELP, "Help"},
  {MENU_EXIT, "Exit"}
};

MENU(main_menu, "PocketTRS");

static ScreenBuffer* screenBuffer;

extern fabgl::PS2Controller PS2Controller;

static void screen_update(uint8_t* from, uint8_t* to)
{
  screenBuffer->update(from, to);
}

static char get_next_key()
{
  auto keyboard = PS2Controller.keyboard();

  while (true) {
    if (!keyboard->virtualKeyAvailable()) {
      continue;
    }
  
    bool down;
    auto vk = keyboard->getNextVirtualKey(&down);
    if (!down) {
      continue;
    }
    switch(vk) {
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
      int c = keyboard->virtualKeyToASCII(vk);
      if (c > -1) {
	return c;
      }
      break;
    }
  }
}

void configure_pocket_trs()
{
  bool show_from_left = false;
  bool exit = false;
  
  screenBuffer = new ScreenBuffer(64, 16);
  trs_screen.push(screenBuffer);
  screenBuffer->copyBufferFrom(screenBuffer->getNext());

  ScreenBuffer* backgroundBuffer = new ScreenBuffer(64, 16);
  trs_screen.push(backgroundBuffer);

  set_screen(screenBuffer->getBuffer(), backgroundBuffer->getBuffer(),
	     64, 16);

  set_screen_callback(screen_update);
  set_keyboard_callback(get_next_key);
  
  while (!exit) {
    uint8_t action = menu(&main_menu, show_from_left, true);
    switch (action) {
    case MENU_CONFIGURE:
      configure();
      break;
    case MENU_STATUS:
      status();
      break;
    case MENU_RESET:
      reset_settings();
      storage_erase();
      esp_restart();
      break;
    case MENU_HELP:
      help();
      break;
    case MENU_EXIT:
    case MENU_ABORT:
      exit = true;
      break;
    }
    show_from_left = true;
  }

  // Copy original screen content of the TRS emulation
  // to the background buffer
  backgroundBuffer->copyBufferFrom(screenBuffer->getNext());
  screen_show(true);
  trs_screen.pop();
  trs_screen.pop();
}
