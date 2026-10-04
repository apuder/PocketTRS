
#include "bluetooth.h"
#include "http.h"
#include "cJSON.h"
#include "esp_attr.h"
#include <string.h>

/*
 * The Bluetooth keyboard in TRS-IO's web UI (Settings):
 *
 *   GET  /bt/status
 *   POST /bt/scan              look for keyboards in pairing mode
 *   POST /bt/pair   {"addr"}   pair with one of them
 *   POST /bt/unpair            forget the paired keyboard
 *
 * All of them answer with the status. Scanning and pairing take a while:
 * the web UI keeps asking for the status until "activity" is 0 again.
 */

static char* status_json()
{
  // Not on the web server's stack
  static bt_kbd_status_t status EXT_RAM_BSS_ATTR;

  bt_kbd_get_status(&status);

  cJSON* s = cJSON_CreateObject();
  cJSON_AddBoolToObject(s, "available", status.available);
  cJSON_AddBoolToObject(s, "paired", status.paired);
  cJSON_AddBoolToObject(s, "connected", status.connected);
  cJSON_AddStringToObject(s, "name", status.name);
  cJSON_AddNumberToObject(s, "activity", status.activity);
  cJSON_AddNumberToObject(s, "passkey", status.passkey);
  cJSON_AddStringToObject(s, "message", status.message);
  cJSON* found = cJSON_AddArrayToObject(s, "found");
  for (int i = 0; i < status.found_count; i++) {
    cJSON* kbd = cJSON_CreateObject();
    cJSON_AddStringToObject(kbd, "addr", status.found[i].addr);
    cJSON_AddStringToObject(kbd, "name", status.found[i].name);
    cJSON_AddNumberToObject(kbd, "rssi", status.found[i].rssi);
    cJSON_AddItemToArray(found, kbd);
  }

  char* response = cJSON_PrintUnformatted(s);
  cJSON_Delete(s);
  return response;
}

// Called on the web server's task, see http_set_host_handler()
static char* handle_request(const char* method, const char* uri, const char* body)
{
  if (strncmp(uri, "/bt/", 4) != 0) {
    return NULL;
  }
  const bool post = strcasecmp(method, "POST") == 0;

  if (post && strcmp(uri, "/bt/scan") == 0) {
    bt_kbd_scan();
  } else if (post && strcmp(uri, "/bt/pair") == 0) {
    cJSON* json = cJSON_Parse(body);
    cJSON* addr = cJSON_GetObjectItemCaseSensitive(json, "addr");
    if (cJSON_IsString(addr) && addr->valuestring != NULL) {
      bt_kbd_pair(addr->valuestring);
    }
    cJSON_Delete(json);
  } else if (post && strcmp(uri, "/bt/unpair") == 0) {
    bt_kbd_unpair();
  } else if (strcmp(uri, "/bt/status") != 0) {
    return NULL;
  }
  return status_json();
}

void init_bluetooth_web()
{
  http_set_host_handler(handle_request);
}
