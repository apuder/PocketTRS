
#include "bluetooth.h"
#include "bt_keyboard.hpp"
#include "fabgl.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_attr.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <string.h>
#include <stdio.h>

#define TAG "BT"

#define BT_SCAN_SECONDS 5

// The stack stores a pairing as keys and an address; the keyboard's name is
// kept here for the web UI.
#define BT_NVS_NAMESPACE "ptrs"
#define BT_NVS_KEY_NAME "bt_name"

extern fabgl::PS2Controller PS2Controller;

static BTKeyboard bt_keyboard;


/*****************************************
 * HID reports to FabGL virtual keys
 *****************************************/

using fabgl::VirtualKey;

// HID usage IDs of a US keyboard, from 0x28 (ENTER) on. Letters (0x04-0x1D)
// and digits (0x1E-0x27) are handled in usage_to_vk().
typedef struct {
  VirtualKey vk;
  VirtualKey shifted;
} hid_key_t;

#define HID_FIRST_KEY 0x28
#define SAME(vk) {fabgl::vk, fabgl::vk}

static const hid_key_t hid_keys[] = {
  SAME(VK_RETURN),                              // 0x28
  SAME(VK_ESCAPE),                              // 0x29
  SAME(VK_BACKSPACE),                           // 0x2A
  SAME(VK_TAB),                                 // 0x2B
  SAME(VK_SPACE),                               // 0x2C
  {fabgl::VK_MINUS, fabgl::VK_UNDERSCORE},      // 0x2D
  {fabgl::VK_EQUALS, fabgl::VK_PLUS},           // 0x2E
  {fabgl::VK_LEFTBRACKET, fabgl::VK_LEFTBRACE}, // 0x2F
  {fabgl::VK_RIGHTBRACKET, fabgl::VK_RIGHTBRACE}, // 0x30
  {fabgl::VK_BACKSLASH, fabgl::VK_VERTICALBAR}, // 0x31
  {fabgl::VK_HASH, fabgl::VK_TILDE},            // 0x32 (non-US)
  {fabgl::VK_SEMICOLON, fabgl::VK_COLON},       // 0x33
  {fabgl::VK_QUOTE, fabgl::VK_QUOTEDBL},        // 0x34
  {fabgl::VK_GRAVEACCENT, fabgl::VK_TILDE},     // 0x35
  {fabgl::VK_COMMA, fabgl::VK_LESS},            // 0x36
  {fabgl::VK_PERIOD, fabgl::VK_GREATER},        // 0x37
  {fabgl::VK_SLASH, fabgl::VK_QUESTION},        // 0x38
  SAME(VK_CAPSLOCK),                            // 0x39
  SAME(VK_F1), SAME(VK_F2), SAME(VK_F3), SAME(VK_F4),   // 0x3A
  SAME(VK_F5), SAME(VK_F6), SAME(VK_F7), SAME(VK_F8),
  SAME(VK_F9), SAME(VK_F10), SAME(VK_F11), SAME(VK_F12),
  SAME(VK_PRINTSCREEN),                         // 0x46
  SAME(VK_SCROLLLOCK),                          // 0x47
  SAME(VK_PAUSE),                               // 0x48
  SAME(VK_INSERT),                              // 0x49
  SAME(VK_HOME),                                // 0x4A
  SAME(VK_PAGEUP),                              // 0x4B
  SAME(VK_DELETE),                              // 0x4C
  SAME(VK_END),                                 // 0x4D
  SAME(VK_PAGEDOWN),                            // 0x4E
  SAME(VK_RIGHT),                               // 0x4F
  SAME(VK_LEFT),                                // 0x50
  SAME(VK_DOWN),                                // 0x51
  SAME(VK_UP),                                  // 0x52
  SAME(VK_NUMLOCK),                             // 0x53
  SAME(VK_KP_DIVIDE),                           // 0x54
  SAME(VK_KP_MULTIPLY),                         // 0x55
  SAME(VK_KP_MINUS),                            // 0x56
  SAME(VK_KP_PLUS),                             // 0x57
  SAME(VK_KP_ENTER),                            // 0x58
  SAME(VK_KP_1), SAME(VK_KP_2), SAME(VK_KP_3), SAME(VK_KP_4), SAME(VK_KP_5),  // 0x59
  SAME(VK_KP_6), SAME(VK_KP_7), SAME(VK_KP_8), SAME(VK_KP_9), SAME(VK_KP_0),
  SAME(VK_KP_PERIOD),                           // 0x63
};

static const VirtualKey shifted_digits[] = {
  fabgl::VK_EXCLAIM, fabgl::VK_AT, fabgl::VK_HASH, fabgl::VK_DOLLAR,
  fabgl::VK_PERCENT, fabgl::VK_CARET, fabgl::VK_AMPERSAND, fabgl::VK_ASTERISK,
  fabgl::VK_LEFTPAREN, fabgl::VK_RIGHTPAREN
};

// Bits of the report's modifier byte
static const VirtualKey modifier_keys[8] = {
  fabgl::VK_LCTRL, fabgl::VK_LSHIFT, fabgl::VK_LALT, fabgl::VK_LGUI,
  fabgl::VK_RCTRL, fabgl::VK_RSHIFT, fabgl::VK_RALT, fabgl::VK_RGUI
};

#define MODIFIER_SHIFT_MASK 0x22

// A keyboard report: the modifier keys, a reserved byte and up to six keys
#define REPORT_FIRST_KEY 2
#define REPORT_MAX_KEYS 6

#define HID_KEY_ROLLOVER 0x01
#define HID_KEY_CAPS_LOCK 0x39

static uint8_t pressed_modifiers = 0;
static uint8_t pressed_keys[REPORT_MAX_KEYS] = {0};
// The virtual key each pressed key was reported as: the same one is
// released, whatever SHIFT is by then.
static VirtualKey pressed_vks[REPORT_MAX_KEYS] = {fabgl::VK_NONE};
static bool caps_lock = false;

// Like FabGL does for a PS/2 keyboard, SHIFT is part of the virtual key:
// VK_a or VK_A, VK_1 or VK_EXCLAIM.
static VirtualKey usage_to_vk(uint8_t usage, bool shift)
{
  if (usage >= 0x04 && usage <= 0x1d) {
    return (VirtualKey) ((shift != caps_lock ? fabgl::VK_A : fabgl::VK_a) + (usage - 0x04));
  }
  if (usage >= 0x1e && usage <= 0x27) {
    if (shift) {
      return shifted_digits[usage - 0x1e];
    }
    // 1 to 9, then 0
    return (usage == 0x27) ? fabgl::VK_0 : (VirtualKey) (fabgl::VK_1 + (usage - 0x1e));
  }
  if (usage >= HID_FIRST_KEY && usage - HID_FIRST_KEY < sizeof(hid_keys) / sizeof(hid_keys[0])) {
    const hid_key_t* key = &hid_keys[usage - HID_FIRST_KEY];
    return shift ? key->shifted : key->vk;
  }
  return fabgl::VK_NONE;
}

static void inject(VirtualKey vk, bool down)
{
  auto keyboard = PS2Controller.keyboard();
  if (keyboard != nullptr && vk != fabgl::VK_NONE) {
    keyboard->injectVirtualKey(vk, down, false);
  }
}

static bool contains(const uint8_t* keys, uint8_t key)
{
  for (int i = 0; i < REPORT_MAX_KEYS; i++) {
    if (keys[i] == key) {
      return true;
    }
  }
  return false;
}

// Turns the difference to the previous report into key presses and
// releases. Called with every report of the keyboard, on a Bluetooth task.
static void on_report(const BTKeyboard::KeyInfo& report)
{
  if (report.size < REPORT_FIRST_KEY) {
    return;
  }
  const uint8_t modifiers = report.keys[0];
  uint8_t keys[REPORT_MAX_KEYS] = {0};
  for (int i = 0; i < REPORT_MAX_KEYS && REPORT_FIRST_KEY + i < report.size; i++) {
    keys[i] = report.keys[REPORT_FIRST_KEY + i];
    if (keys[i] == HID_KEY_ROLLOVER) {
      // Too many keys pressed
      return;
    }
  }

  // Modifiers go down before the keys they apply to and up after them
  const uint8_t changed = modifiers ^ pressed_modifiers;
  for (int i = 0; i < 8; i++) {
    if ((changed & modifiers) & (1 << i)) {
      inject(modifier_keys[i], true);
    }
  }

  for (int i = 0; i < REPORT_MAX_KEYS; i++) {
    if (pressed_keys[i] != 0 && !contains(keys, pressed_keys[i])) {
      inject(pressed_vks[i], false);
      pressed_keys[i] = 0;
    }
  }

  const bool shift = (modifiers & MODIFIER_SHIFT_MASK) != 0;
  for (int i = 0; i < REPORT_MAX_KEYS; i++) {
    if (keys[i] == 0 || contains(pressed_keys, keys[i])) {
      continue;
    }
    if (keys[i] == HID_KEY_CAPS_LOCK) {
      caps_lock = !caps_lock;
    }
    for (int j = 0; j < REPORT_MAX_KEYS; j++) {
      if (pressed_keys[j] == 0) {
        pressed_keys[j] = keys[i];
        pressed_vks[j] = usage_to_vk(keys[i], shift);
        inject(pressed_vks[j], true);
        break;
      }
    }
  }

  for (int i = 0; i < 8; i++) {
    if ((changed & ~modifiers) & (1 << i)) {
      inject(modifier_keys[i], false);
    }
  }
  pressed_modifiers = modifiers;
}

// The keyboard is gone: nothing is pressed any more
static void release_all()
{
  BTKeyboard::KeyInfo empty;
  memset(&empty, 0, sizeof(empty));
  empty.size = REPORT_FIRST_KEY + REPORT_MAX_KEYS;
  on_report(empty);
}


/*****************************************
 * Pairing and connecting
 *****************************************/

enum bt_command_type_t {
  BT_CMD_SCAN,
  BT_CMD_PAIR,
  BT_CMD_UNPAIR
};

struct bt_command_t {
  bt_command_type_t type;
  char addr[18];
};

static QueueHandle_t commands;

// Guards state and found
static SemaphoreHandle_t lock;
static bt_kbd_status_t state EXT_RAM_BSS_ATTR;
static BTKeyboard::Keyboard found[BT_KBD_MAX_FOUND] EXT_RAM_BSS_ATTR;

static void name_load(char* name, size_t len)
{
  nvs_handle_t h;

  name[0] = '\0';
  if (nvs_open(BT_NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK) {
    return;
  }
  if (nvs_get_str(h, BT_NVS_KEY_NAME, name, &len) != ESP_OK) {
    name[0] = '\0';
  }
  nvs_close(h);
}

static void name_save(const char* name)
{
  nvs_handle_t h;

  if (nvs_open(BT_NVS_NAMESPACE, NVS_READWRITE, &h) != ESP_OK) {
    return;
  }
  if (name != NULL) {
    nvs_set_str(h, BT_NVS_KEY_NAME, name);
  } else {
    nvs_erase_key(h, BT_NVS_KEY_NAME);
  }
  nvs_commit(h);
  nvs_close(h);
}

static void addr_to_str(const esp_bd_addr_t bda, char* str, size_t len)
{
  snprintf(str, len, "%02x:%02x:%02x:%02x:%02x:%02x",
           bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
}

// These three run on Bluetooth tasks

// Some keyboards pair with a passkey: the user types it on the keyboard
static void on_passkey(uint32_t passkey)
{
  ESP_LOGI(TAG, "Pairing code: %06u", (unsigned) passkey);
  state.passkey = passkey;
}

static void on_connected()
{
  ESP_LOGI(TAG, "Keyboard connected");
  state.connected = true;
}

static void on_disconnected()
{
  ESP_LOGI(TAG, "Keyboard disconnected");
  state.connected = false;
  release_all();
}

static void set_message(const char* fmt, const char* name)
{
  xSemaphoreTake(lock, portMAX_DELAY);
  snprintf(state.message, sizeof(state.message), fmt, name);
  state.activity = BT_KBD_IDLE;
  state.passkey = 0;
  xSemaphoreGive(lock);
}

// Internal memory is scarce: VGA, Wi-Fi, the Bluetooth controller and every
// task's stack need it (see sdkconfig.defaults).
static void log_memory(const char* when)
{
  ESP_LOGI(TAG, "%s: %u bytes of internal memory free (largest block %u)", when,
           (unsigned) heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
           (unsigned) heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
}

// The keyboard's address is the one it had when it was paired. A Logitech
// keyboard takes a new one each time it is put in pairing mode: then this
// pairing is of no use any more.
static void log_paired()
{
  static esp_ble_bond_dev_t devices[2];
  int n = sizeof(devices) / sizeof(devices[0]);

  if (esp_ble_get_bond_device_list(&n, devices) != ESP_OK) {
    return;
  }
  for (int i = 0; i < n; i++) {
    ESP_LOGI(TAG, "Paired keyboard: " ESP_BD_ADDR_STR " (address type %d)",
             ESP_BD_ADDR_HEX(devices[i].bd_addr), devices[i].bd_addr_type);
  }
}

static void scan()
{
  static BTKeyboard::Keyboard results[BT_KBD_MAX_FOUND] EXT_RAM_BSS_ATTR;

  const int n = bt_keyboard.scan_keyboards(results, BT_KBD_MAX_FOUND, BT_SCAN_SECONDS);

  xSemaphoreTake(lock, portMAX_DELAY);
  memcpy(found, results, sizeof(found));
  state.found_count = n;
  for (int i = 0; i < n; i++) {
    addr_to_str(found[i].bda, state.found[i].addr, sizeof(state.found[i].addr));
    strlcpy(state.found[i].name, found[i].name, sizeof(state.found[i].name));
    state.found[i].rssi = found[i].rssi;
  }
  xSemaphoreGive(lock);
  set_message(n == 0 ? "No keyboards found" : "", "");
}

static void pair(const char* addr)
{
  BTKeyboard::Keyboard kbd;
  bool known = false;

  xSemaphoreTake(lock, portMAX_DELAY);
  for (int i = 0; i < state.found_count; i++) {
    if (strcmp(state.found[i].addr, addr) == 0) {
      kbd = found[i];
      known = true;
    }
  }
  xSemaphoreGive(lock);
  if (!known) {
    set_message("Scan for the keyboard first", "");
    return;
  }

  ESP_LOGI(TAG, "Pairing with '%s'", kbd.name);
  if (bt_keyboard.connect(kbd)) {
    name_save(kbd.name);
    xSemaphoreTake(lock, portMAX_DELAY);
    strlcpy(state.name, kbd.name, sizeof(state.name));
    state.paired = true;
    state.found_count = 0;
    xSemaphoreGive(lock);
    // The web UI shows the keyboard as connected
    set_message("", "");
  } else {
    ESP_LOGW(TAG, "Pairing with '%s' failed", kbd.name);
    set_message("Could not pair with %s", kbd.name);
  }
  log_memory("After pairing");
}

static void unpair()
{
  bt_keyboard.unpair_all();
  name_save(NULL);
  xSemaphoreTake(lock, portMAX_DELAY);
  state.paired = false;
  state.name[0] = '\0';
  xSemaphoreGive(lock);
  set_message("", "");
}

// Does what the web UI asks for and, in between, keeps the paired keyboard
// connected: whenever it is not, try to reach it. An attempt blocks until
// the keyboard answers or the BLE connection times out
// (CONFIG_BT_BLE_ESTAB_LINK_CONN_TOUT), so a keyboard that wakes up (they
// sleep when idle) is back within seconds. With no keyboard paired the radio
// stays quiet.
static void bt_task(void* arg)
{
  log_memory("Before Bluetooth");
  if (!bt_keyboard.setup(on_passkey, on_connected, on_disconnected)) {
    ESP_LOGE(TAG, "Bluetooth setup failed: no Bluetooth keyboard");
    vTaskDelete(NULL);
    return;
  }
  bt_keyboard.set_report_sink(on_report);

  xSemaphoreTake(lock, portMAX_DELAY);
  state.paired = bt_keyboard.paired_count() > 0;
  if (state.paired) {
    name_load(state.name, sizeof(state.name));
    if (state.name[0] == '\0') {
      strlcpy(state.name, "Keyboard", sizeof(state.name));
    }
  }
  state.available = true;
  xSemaphoreGive(lock);
  log_paired();
  log_memory("Bluetooth is up");

  while (true) {
    bt_command_t command;

    if (xQueueReceive(commands, &command, 3000 / portTICK_PERIOD_MS) == pdTRUE) {
      switch (command.type) {
      case BT_CMD_SCAN:
        scan();
        break;
      case BT_CMD_PAIR:
        pair(command.addr);
        break;
      case BT_CMD_UNPAIR:
        unpair();
        break;
      }
    } else if (state.paired && !bt_keyboard.is_connected()) {
      bt_keyboard.connect_paired();
    }
  }
}

// One keyboard at a time: scanning and pairing are for when none is paired
static bool send_command(const bt_command_t& command, bt_kbd_activity_t activity)
{
  bool ok = false;

  xSemaphoreTake(lock, portMAX_DELAY);
  if (state.available && state.activity == BT_KBD_IDLE &&
      (command.type == BT_CMD_UNPAIR || !state.paired) &&
      xQueueSend(commands, &command, 0) == pdTRUE) {
    // Until the task is done with it, which can be after an attempt to
    // reach the paired keyboard
    state.activity = activity;
    state.message[0] = '\0';
    ok = true;
  }
  xSemaphoreGive(lock);
  return ok;
}

bool bt_kbd_scan()
{
  bt_command_t command = {BT_CMD_SCAN, ""};
  return send_command(command, BT_KBD_SCANNING);
}

bool bt_kbd_pair(const char* addr)
{
  bt_command_t command = {BT_CMD_PAIR, ""};
  strlcpy(command.addr, addr, sizeof(command.addr));
  return send_command(command, BT_KBD_PAIRING);
}

bool bt_kbd_unpair()
{
  bt_command_t command = {BT_CMD_UNPAIR, ""};
  // Shown like a scan: it is over at once
  return send_command(command, BT_KBD_SCANNING);
}

void bt_kbd_get_status(bt_kbd_status_t* status)
{
  if (lock == NULL) {
    memset(status, 0, sizeof(*status));
    return;
  }
  xSemaphoreTake(lock, portMAX_DELAY);
  *status = state;
  xSemaphoreGive(lock);
}

void init_bluetooth()
{
  lock = xSemaphoreCreateMutex();
  commands = xQueueCreate(1, sizeof(bt_command_t));
  xTaskCreatePinnedToCore(bt_task, "bt", 6000, NULL, 1, NULL, 1);
}
