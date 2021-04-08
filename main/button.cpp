
#include "button.h"
#include "driver/gpio.h"
#include "esp_event.h"

#define GPIO_BUTTON GPIO_NUM_35

#define ESP_INTR_FLAG_DEFAULT 0

#define BUTTON_LONG_PRESS BIT0
#define BUTTON_SHORT_PRESS BIT1

static volatile uint8_t button_status = 0;


bool is_button_pressed()
{
  return (GPIO.in1.data & (1 << (GPIO_BUTTON - 32))) == 0;
}

bool is_button_long_press()
{
  bool yes = (button_status & BUTTON_LONG_PRESS) != 0;
  if (yes) {
    button_status &= ~BUTTON_LONG_PRESS;
  }
  return yes;
}

bool is_button_short_press()
{
  bool yes = (button_status & BUTTON_SHORT_PRESS) != 0;
  if (yes) {
    button_status &= ~BUTTON_SHORT_PRESS;
  }
  return yes;
}

static void IRAM_ATTR isr_button(void* arg)
{
  static int64_t then;

  if (is_button_pressed()) {
    then = esp_timer_get_time();
  } else {
    int64_t now = esp_timer_get_time();
    int64_t delta_ms = (now - then) / 1000;
    if (delta_ms < 50) {
      // Bounce
      return;
    }
    if (delta_ms < 500) {
      button_status |= BUTTON_SHORT_PRESS;
    }
    if (delta_ms > 3000) {
      button_status |= BUTTON_LONG_PRESS;
    }
  }
}

void init_button()
{
  gpio_config_t gpioConfig;

  // Configure push button
  gpioConfig.pin_bit_mask = (1ULL << GPIO_BUTTON);
  gpioConfig.mode = GPIO_MODE_INPUT;
  gpioConfig.pull_up_en = GPIO_PULLUP_DISABLE;
  gpioConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpioConfig.intr_type = GPIO_INTR_ANYEDGE;
  gpio_config(&gpioConfig);
  gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
  gpio_isr_handler_add(GPIO_BUTTON, isr_button, NULL);
}
