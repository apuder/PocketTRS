
#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/i2c.h>
#include "config.h"
#include "i2c.h"


static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
  xQueueSendFromISR(gpio_evt_queue, NULL, NULL);
}

void writeMCP(uint8_t addr, uint8_t cmd, uint8_t data)
{
  i2c_cmd_handle_t cmd_h = i2c_cmd_link_create();
  i2c_master_start(cmd_h);
  i2c_master_write_byte(cmd_h, (addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
  i2c_master_write_byte(cmd_h, cmd, ACK_CHECK_EN);
  i2c_master_write_byte(cmd_h, data, ACK_CHECK_EN);
  i2c_master_stop(cmd_h);
  i2c_master_cmd_begin(I2C_NUM_0, cmd_h, 1000 / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd_h);
}

void writeMCP(uint8_t addr, uint8_t cmd, uint8_t data1, uint8_t data2)
{
  i2c_cmd_handle_t cmd_h = i2c_cmd_link_create();
  i2c_master_start(cmd_h);
  i2c_master_write_byte(cmd_h, (addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
  i2c_master_write_byte(cmd_h, cmd, ACK_CHECK_EN);
  i2c_master_write_byte(cmd_h, data1, ACK_CHECK_EN);
  i2c_master_write_byte(cmd_h, data2, ACK_CHECK_EN);
  i2c_master_stop(cmd_h);
  i2c_master_cmd_begin(I2C_NUM_0, cmd_h, 1000 / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd_h);
}

uint8_t readMCP(uint8_t addr, uint8_t reg)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);

  cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
  uint8_t value;
  i2c_master_read_byte(cmd, &value, I2C_MASTER_ACK);
  i2c_master_stop(cmd);
  i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);

  return value;
}

#if 0
void wire_test()
{

  init_i2c();
  delay(1000);
  //Wire.begin(GPIO_SDA, GPIO_SCL);
  //Wire.setClock(400000);

#if 1
  while (1) {
    if(xQueueReceive(gpio_evt_queue, NULL, portMAX_DELAY)) {
      uint8_t data = readMCP2(MCP23008_INTCAP);
      Serial.println(data);
    }
  }
#else
  uint8_t data = 0;
  while (1) {
    writeMCP1(MCP23017_GPIOB, data);
//    writeMCP2(MCP23008_GPIO, data);
    delay(500);
    data ^= 0xff;
    writeMCP1(MCP23017_GPIOB, data);
//    writeMCP2(MCP23008_GPIO, data);
    delay(500);
    data ^= 0xff;
  }
#endif
}
#endif

void init_i2c()
{
  // Configure I2C
  i2c_config_t conf;
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = (gpio_num_t) MCP_GPIO_SDA;
  conf.scl_io_num = (gpio_num_t) MCP_GPIO_SCL;
  conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
  conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
  conf.master.clk_speed = 400000;
  i2c_param_config(I2C_NUM_0, &conf);
  i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);

  // Configure ESP's MCP23008 INT GPIO
  gpio_config_t io_conf;
  //disable interrupt
  io_conf.intr_type = GPIO_INTR_DISABLE; //GPIO_INTR_NEGEDGE;
  //set as output mode
  io_conf.mode = GPIO_MODE_INPUT;
  //bit mask of the pins that you want to set,e.g.GPIO18/19
  io_conf.pin_bit_mask = GPIO_SEL_18;
  //disable pull-down mode
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  //disable pull-up mode
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  //configure GPIO with the given settings
  gpio_config(&io_conf);

#if 0
  gpio_install_isr_service(0);
  //hook isr handler for specific gpio pin
  gpio_isr_handler_add(GPIO_NUM_18, gpio_isr_handler, NULL);

  gpio_evt_queue = xQueueCreate(10, 0);
#endif

  /*
   * MCP23017 configuration
   */
  // Port B is connected to D0-D7. Configure as input. Disable pull-ups.
  // Disable interrupts
  writeMCP(MCP23017_ADDRESS, MCP23017_IODIRB, 0xff);
  writeMCP(MCP23017_ADDRESS, MCP23017_GPPUB, 0);
  writeMCP(MCP23017_ADDRESS, MCP23017_GPINTENB, 0);
  // Port A is connected to A0-A7. Configure as output. Set 0 as initial address
  writeMCP(MCP23017_ADDRESS, MCP23017_IODIRA, 0);
  writeMCP(MCP23017_ADDRESS, MCP23017_GPIOA, 0);

  /*
   * MCP23008 configuration
   */
  // Configure as input: TRS_IOBUSINT, TRS_IOBUSWAIT, TRS_EXTIOSEL
  writeMCP(MCP23008_ADDRESS, MCP23008_IODIR, TRS_IOBUSINT | TRS_IOBUSWAIT | TRS_EXTIOSEL);
  // Enable pull-ups
  writeMCP(MCP23008_ADDRESS, MCP23008_GPPU, 0xff);
  // Configure INT as open drain
  writeMCP(MCP23008_ADDRESS, MCP23008_IOCON, MCP23008_ODR);
#if 0
  // Generate interrupt on pin change
  writeMCP(MCP23008_ADDRESS, MCP23008_INTCON, 0);
  // Enable interrupt-on-change
  writeMCP(MCP23008_ADDRESS, MCP23008_GPINTEN, 0xff);
  // Dummy read to clear INT
  readMCP(MCP23008_ADDRESS, MCP23008_INTCAP);
#endif
}
