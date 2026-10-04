file(GLOB app_sources "*.cpp" "*.c")

idf_component_register(SRCS ${app_sources}
                       INCLUDE_DIRS .
                       REQUIRES trs-io
                                trs-lib
                                FabGL
                                freertos
                                nvs_flash
                                esp_timer
                                esp_adc
                                esp_driver_dac
                                esp_driver_gpio
                                esp_driver_spi)
