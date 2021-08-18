
#include "wifi.h"
#include "trs-fs.h"
#include "version-ptrs.h"
#include "version.h"

extern "C" {
#include "trs-lib.h"
}

static window_t wnd;

void status()
{
  set_screen_to_background();
  init_window(&wnd, 0, 3, 0, 0);
  header("Status");
  wnd_print(&wnd, false, "PocketTRS version: ");
  wnd_print_int(&wnd, POCKET_TRS_VERSION_MAJOR);
  wnd_print(&wnd, false, ".");
  wnd_print_int(&wnd, POCKET_TRS_VERSION_MINOR);
  wnd_cr(&wnd);

  wnd_print(&wnd, false, "TRS-IO version   : ");
  wnd_print_int(&wnd, TRS_IO_VERSION_MAJOR);
  wnd_print(&wnd, false, ".");
  wnd_print_int(&wnd, TRS_IO_VERSION_MINOR);
  wnd_cr(&wnd);
  wnd_cr(&wnd);

  const char* status;
  switch(*get_wifi_status()) {
  case RS_STATUS_WIFI_NOT_NEEDED:
    status = "emulation mode";
    break;
  case RS_STATUS_WIFI_CONNECTING:
    status = "WiFi connecting";
    break;
  case RS_STATUS_WIFI_CONNECTED:
    status = "WiFi connected";
    break;
  case RS_STATUS_WIFI_NOT_CONNECTED:
    status = "WiFi not connected";
    break;
  case RS_STATUS_WIFI_NOT_CONFIGURED:
    status = "WiFi not configured";
    break;
  case RS_STATUS_NO_RETROSTORE_CARD:
    status = "not present";
    break;
  default:
    status = "";
  }
  wnd_print(&wnd, false, "TRS-IO online status: ");
  wnd_print(&wnd, false, status);
  wnd_cr(&wnd);

  wnd_print(&wnd, false, "WiFi SSID           : ");
  wnd_print(&wnd, false, get_wifi_ssid());
  wnd_cr(&wnd);

  wnd_print(&wnd, false, "WiFi IP             : ");
  wnd_print(&wnd, false, get_wifi_ip());
  wnd_cr(&wnd);
  wnd_cr(&wnd);

  wnd_print(&wnd, false, "SMB status: ");
  const char* smb_err = get_smb_err_msg();
  wnd_print(&wnd, false, smb_err == NULL ? "SMB connected" : smb_err);

  if (trs_fs_has_sd_card_reader()) {
    wnd_cr(&wnd);
    wnd_print(&wnd, false, "SD status : ");
    const char* posix_err = get_posix_err_msg();
    wnd_print(&wnd, false, posix_err == NULL ? "Mounted" : posix_err);
  }

  screen_show(false);
  get_key();
}
