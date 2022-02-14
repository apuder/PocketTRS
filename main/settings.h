
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

    static int8_t nvs_get_i8(const char* key) {
        int8_t value = 0;
        esp_err_t err = ::nvs_get_i8(storage, key, &value);
        assert(err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND);
        return value;
    }

    static void nvs_set_i8(const char* key, int8_t value) {
        ESP_ERROR_CHECK(::nvs_set_i8(storage, key, value));
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
 * SettingsROM
 ****************************************************************/

enum rom_type_t {
    ROM_FREHD = 0,
    ROM_XROM
};

class SettingsROM : public virtual SettingsBase {
    public:
    void init();
    rom_type_t getROMType();
    void setROMType(rom_type_t rom);
};

extern SettingsROM settingsROM;


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


/****************************************************************
 * SettingsSplashScreen
 ****************************************************************/

class SettingsSplashScreen : public virtual SettingsBase {
    private:
    bool hide_splash_screen;
    public:
    void init();
    bool hideSplashScreen();
    void hideSplashScreen(bool hide);
};

extern SettingsSplashScreen settingsSplashScreen;


/****************************************************************
 * SettingsCalibration
 ****************************************************************/

class SettingsCalibration : public virtual SettingsBase {
    private:
    int8_t screenOffsetX;
    int8_t screenOffsetY;
    public:
    void init();
    int8_t getScreenOffsetX() { return screenOffsetX; }
    int8_t getScreenOffsetY() { return screenOffsetY; }
    void setScreenOffset();
    void moveScreenOffset(int8_t dx, int8_t dy);
    void saveScreenOffset();
};

extern SettingsCalibration settingsCalibration;


void init_settings();
