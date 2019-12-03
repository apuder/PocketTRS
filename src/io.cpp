

#include <Arduino.h>
#include "driver/gpio.h"
#include "config.h"


// GPIO pins 12-19
#define GPIO_DATA_BUS_MASK 0b11111111000000000000

#define GPIO_OUTPUT_DISABLE(mask) GPIO.enable_w1tc = (mask)

#define GPIO_OUTPUT_ENABLE(mask) GPIO.enable_w1ts = (mask)


static void write_shifter(uint8_t flags, uint8_t address)
{
  digitalWrite(SHIFTER_LATCH, LOW);
  flags ^= 0xff;
  shiftOut(SHIFTER_DATA, SHIFTER_CLOCK, LSBFIRST, address);
  shiftOut(SHIFTER_DATA, SHIFTER_CLOCK, LSBFIRST, flags);
  digitalWrite(SHIFTER_LATCH, HIGH);
}

void z80_out(uint8_t address, uint8_t data)
{
  #if 1
  Serial.print("out(");
  Serial.print(address);
  Serial.print(",");
  Serial.print(data);
  Serial.println(")");
  #endif
  // Write data to GPIO pins 12-19
  GPIO_OUTPUT_ENABLE(GPIO_DATA_BUS_MASK);
  uint32_t d = data;
  d <<= 12;
  REG_WRITE(GPIO_OUT_W1TS_REG, d);
  d = d ^ GPIO_DATA_BUS_MASK;
  REG_WRITE(GPIO_OUT_W1TC_REG, d);

  write_shifter(TRS_ENEXTIO | TRS_IORQ | TRS_OUT, address);
  delay(2);
  //while(digitalRead(TRS_IOBUSWAIT) == LOW) ;
  write_shifter(0, 0);
  GPIO_OUTPUT_DISABLE(GPIO_DATA_BUS_MASK);
}

uint8_t z80_in(uint8_t address)
{
  write_shifter(TRS_ENEXTIO | TRS_IORQ | TRS_IN, address);
  delay(2);
  //while(digitalRead(TRS_IOBUSWAIT) == LOW) ;
  uint8_t data = GPIO.in >> 12;
  write_shifter(0, 0);
  #if 1
  Serial.print("in(");
  Serial.print(address);
  Serial.print("): ");
  Serial.println(data);
  #endif
  return data;
}

void test_io(uint32_t d)
{
  GPIO_OUTPUT_ENABLE(GPIO_DATA_BUS_MASK);
  d <<= 12;
  REG_WRITE(GPIO_OUT_W1TS_REG, d);
  d = d ^ GPIO_DATA_BUS_MASK;
  REG_WRITE(GPIO_OUT_W1TC_REG, d);
}

void init_io()
{
  gpio_config_t gpioConfig;

  // GPIO pins 12-19 (8 pins) are used for data bus
  gpioConfig.pin_bit_mask = GPIO_SEL_12 | GPIO_SEL_13 | GPIO_SEL_14 |
    GPIO_SEL_15 | GPIO_SEL_16 |
    GPIO_SEL_17 | GPIO_SEL_18 | GPIO_SEL_19;
  gpioConfig.mode = GPIO_MODE_INPUT;
  gpioConfig.pull_up_en = GPIO_PULLUP_DISABLE;
  gpioConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpioConfig.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&gpioConfig);

  pinMode(SHIFTER_LATCH, OUTPUT);
  pinMode(SHIFTER_CLOCK, OUTPUT);
  pinMode(SHIFTER_DATA, OUTPUT);

  pinMode(TRS_IOBUSINT, INPUT);
  pinMode(TRS_IOBUSWAIT, INPUT);

  write_shifter(0, 0);

#if 0
z80_out(0xc5, 3);
for (int i = 0; i < 10; i++) {
  Serial.println(z80_in(0xc4));
}

while(1);
#endif
}
