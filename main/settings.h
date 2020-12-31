
#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include <nvs_flash.h>

//------------------------------------------------------------------
class SettingsBase {
    private:
    static nvs_handle storage;

    protected:
    static uint8_t nvs_get_u8(const char* key) {
        uint8_t value = 0;
        esp_err_t err = ::nvs_get_u8(storage, key, &value);
        assert(err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND);
        return value;
    }

    static void nvs_set_u8(const char* key, uint8_t value) {
        ESP_ERROR_CHECK(::nvs_set_u8(storage, key, value));
        ESP_ERROR_CHECK(nvs_commit(storage));
    }

    public:
    static void init() {
        // Initialize NVS
        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
            // NVS partition was truncated and needs to be erased
            // Retry nvs_flash_init
            ESP_ERROR_CHECK(nvs_flash_erase());
            err = nvs_flash_init();
        }
        ESP_ERROR_CHECK(err);
        err = nvs_open("ptrs", NVS_READWRITE, &storage);
        ESP_ERROR_CHECK(err);
    }

    static void reset() {
        ESP_ERROR_CHECK(nvs_erase_all(storage));
        ESP_ERROR_CHECK(nvs_commit(storage));
    }
};

/****************************************************************
 * SettingsScreen
 ****************************************************************/

enum screen_color_t {
    SCREEN_COLOR_WHITE = 0,
    SCREEN_COLOR_GREEN,
    SCREEN_COLOR_AMBER
};

class SettingsScreen : public virtual SettingsBase {
    public:
    void init();
    screen_color_t getScreenColor();
    void setScreenColor(screen_color_t color);
};

extern SettingsScreen settingsScreen;


/****************************************************************
 * SettingsTrsIO
 ****************************************************************/

class SettingsTrsIO : public virtual SettingsBase {
    private:
    bool use_trs_io;
    public:
    void init();
    bool isEnabled();
    void setEnabled(bool enabled);
};

extern SettingsTrsIO settingsTrsIO;


void init_settings();
