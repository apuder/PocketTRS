
#pragma once

#include <stdint.h>
#include <stdbool.h>

// Bluetooth (BLE) keyboard, next to the PS/2 one: its keys go into the same
// FabGL virtual key queue. One keyboard can be paired; it is paired from
// TRS-IO's web UI (bluetooth_web.cpp) and reconnects on its own afterwards.

#define BT_KBD_MAX_FOUND 8

enum bt_kbd_activity_t {
  BT_KBD_IDLE = 0,
  BT_KBD_SCANNING,
  BT_KBD_PAIRING
};

struct bt_kbd_found_t {
  char addr[18]; // "aa:bb:cc:dd:ee:ff"
  char name[32];
  int8_t rssi;
};

struct bt_kbd_status_t {
  bool available;     // the Bluetooth stack is up
  bool paired;
  bool connected;
  char name[32];      // the paired keyboard, "" if none
  bt_kbd_activity_t activity;
  // While pairing, a keyboard may ask for this code to be typed on it,
  // followed by ENTER. 0: none.
  uint32_t passkey;
  // Result of the last scan or pairing, "" if none
  char message[64];
  // Keyboards seen by the last scan, strongest signal first
  int found_count;
  bt_kbd_found_t found[BT_KBD_MAX_FOUND];
};

// Brings Bluetooth up and keeps the paired keyboard connected
void init_bluetooth();
// Pairing in TRS-IO's web UI
void init_bluetooth_web();

void bt_kbd_get_status(bt_kbd_status_t* status);

// These return at once; bt_kbd_get_status() tells how it went. They return
// false if Bluetooth is not available or busy with a previous request, and
// scanning and pairing also while a keyboard is paired.

// Look for keyboards in pairing mode
bool bt_kbd_scan();
// Pair with a keyboard of the last scan
bool bt_kbd_pair(const char* addr);
// Forget the paired keyboard
bool bt_kbd_unpair();
