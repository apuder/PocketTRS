#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_log.h>
#include <string.h>
#include "trs-fs.h"
#include "button.h"
#include "config.h"
#include "spi.h"

#define MCP23S_WRITE 0x00
#define MCP23S_READ  0x01

static spi_device_interface_config_t spi_mcp23x;
spi_device_handle_t spi_mcp23x_h;

static spi_device_interface_config_t spi_mcp4351;
spi_device_handle_t spi_mcp4351_h;

static xQueueHandle gpio_evt_queue = NULL;


void writePortExpander(uint8_t addr, uint8_t cmd, uint8_t data)
{
  spi_transaction_t trans;

  memset(&trans, 0, sizeof(spi_transaction_t));
  trans.flags = SPI_TRANS_USE_TXDATA;
  trans.length = 3 * 8;   		// 3 bytes
  trans.tx_data[0] = 0x40 | (addr << 1) | MCP23S_WRITE;
  trans.tx_data[1] = cmd;
  trans.tx_data[2] = data;

  esp_err_t ret = spi_device_transmit(spi_mcp23x_h, &trans);
  ESP_ERROR_CHECK(ret);
}

uint8_t readPortExpander(uint8_t addr, uint8_t reg)
{
  spi_transaction_t trans;

  memset(&trans, 0, sizeof(spi_transaction_t));
  trans.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
  trans.length = 3 * 8;   		// 3 bytes
  trans.rxlength = 0;
  trans.tx_data[0] = 0x40 | (addr << 1) | MCP23S_READ;
  trans.tx_data[1] = reg;
  trans.tx_data[2] = 0;

  esp_err_t ret = spi_device_transmit(spi_mcp23x_h, &trans);
  ESP_ERROR_CHECK(ret);

  return trans.rx_data[2];
}

void writeDigiPot(uint8_t pot, uint8_t step)
{
  spi_transaction_t trans;
  const uint8_t cmd = 0; // Write command
  uint8_t p = 0;

  // Determine MCP4351 memory address (table 7-2)
  switch(pot) {
  case 0:
    p = 0;
    break;
  case 1:
    p = 1;
    break;
  case 2:
    p = 6;
    break;
  default:
    assert(0);
  }

  uint8_t data = (p << 4) | (cmd << 2);

  memset(&trans, 0, sizeof(spi_transaction_t));
  trans.flags = SPI_TRANS_USE_TXDATA;
  trans.length = 2 * 8;   		// 2 bytes
  trans.tx_data[0] = data;
  trans.tx_data[1] = step;
  esp_err_t ret = spi_device_transmit(spi_mcp4351_h, &trans);
  ESP_ERROR_CHECK(ret);
}

uint8_t readDigiPot(uint8_t pot)
{
  spi_transaction_t trans;
  const uint8_t cmd = 3; // Read command
  uint8_t p = 0;

  // Determine MCP4351 memory address (table 7-2)
  switch(pot) {
  case 0:
    p = 0;
    break;
  case 1:
    p = 1;
    break;
  case 2:
    p = 6;
    break;
  default:
    assert(0);
  }

  uint8_t data = (p << 4) | (cmd << 2);

  memset(&trans, 0, sizeof(spi_transaction_t));
  trans.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
  trans.length = 2 * 8;   		// 2 bytes
  trans.rxlength = 0;
  trans.tx_data[0] = data;
  trans.tx_data[1] = 0;
  esp_err_t ret = spi_device_transmit(spi_mcp4351_h, &trans);
  ESP_ERROR_CHECK(ret);
  return trans.rx_data[1];
}

void wire_test_port_expander()
{
  // Check POR defaults to see if MCP23S* are present
  bool not_found = false;
  not_found |= readPortExpander(MCP23S17, MCP23S17_IODIRA) != 0xff;
  not_found |= readPortExpander(MCP23S17, MCP23S17_IPOLA) != 0x00;
  not_found |= readPortExpander(MCP23S17, MCP23S17_GPINTENA) != 0x00;
  not_found |= readPortExpander(MCP23S17, MCP23S17_IODIRB) != 0xff;
  not_found |= readPortExpander(MCP23S17, MCP23S17_IPOLB) != 0x00;
  not_found |= readPortExpander(MCP23S17, MCP23S17_GPINTENB) != 0x00;

  if (not_found) {
    ESP_LOGE("SPI", "MCP23S17 not found");
  } else {
    ESP_LOGI("SPI", "MCP23S17 found");
  }
  while(not_found) ;

  not_found = false;
  not_found |= readPortExpander(MCP23S08, MCP23S08_IODIR) != 0xff;
  not_found |= readPortExpander(MCP23S08, MCP23S08_IPOL) != 0x00;
  not_found |= readPortExpander(MCP23S08, MCP23S08_GPINTEN) != 0x00;

  if (not_found) {
    ESP_LOGE("SPI", "MCP23S08 not found");
  } else {
    ESP_LOGI("SPI", "MCP23S08 found");
  }
  while(not_found) ;

  writePortExpander(MCP23S17, MCP23S17_IODIRA, 0);
  writePortExpander(MCP23S17, MCP23S17_IODIRB, 0);
  writePortExpander(MCP23S08, MCP23S08_IODIR, 0);

  uint8_t data = 0xaa;

  while (1) {
    if (is_button_pressed()) {
      return;
    }

    writePortExpander(MCP23S17, MCP23S17_GPIOA, data);
    writePortExpander(MCP23S17, MCP23S17_GPIOB, data);
    writePortExpander(MCP23S08, MCP23S08_GPIO, data);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    data ^= 0xff;
  }
}

static void wire_test_digital_pot()
{
  gpio_config_t gpioConfig;

  gpioConfig.pin_bit_mask = (1ULL << VGA_RED) | (1ULL << VGA_GREEN) | (1ULL << VGA_BLUE);
  gpioConfig.mode = GPIO_MODE_OUTPUT;
  gpioConfig.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&gpioConfig);
  gpio_set_level(VGA_RED, 1);
  gpio_set_level(VGA_GREEN, 1);
  gpio_set_level(VGA_BLUE, 1);

#define VOLTAGE_TO_STEP(v) (int) (255.0f * (v) / 3.3f)
  writeDigiPot(0, VOLTAGE_TO_STEP(1));
  writeDigiPot(1, VOLTAGE_TO_STEP(2));
  writeDigiPot(2, VOLTAGE_TO_STEP(3));

  bool not_found = false;
  not_found |= readDigiPot(0) != VOLTAGE_TO_STEP(1);
  not_found |= readDigiPot(1) != VOLTAGE_TO_STEP(2);
  not_found |= readDigiPot(2) != VOLTAGE_TO_STEP(3);
  if (not_found) {
    ESP_LOGE("SPI", "MCP4361 not found");
  } else {
    ESP_LOGI("SPI", "MCP4361 found");
  }
  while (not_found) ;
}

static void test_sd_card()
{
  ESP_LOGI("SPI", "Testing SD card reader");
  const char* err = init_trs_fs_posix();
  if (err == NULL) {
    ESP_LOGI("SPI", "SD card succefully mounted");
  } else {
    ESP_LOGE("SPI", "Error: %s", err);
  }
  while (true);
}

static void test()
{
  ESP_LOGI("SPI", "PocketTRS SPI bus test");
  wire_test_digital_pot();
  wire_test_port_expander();
  test_sd_card();
}

void init_spi()
{
  spi_bus_config_t bus_cfg = {
    .mosi_io_num = SPI_PIN_NUM_MOSI,
    .miso_io_num = SPI_PIN_NUM_MISO,
    .sclk_io_num = SPI_PIN_NUM_CLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 4000,
  };
  esp_err_t ret = spi_bus_initialize(HSPI_HOST, &bus_cfg, 1);
  if (ret != ESP_OK) {
    ESP_LOGE("SPI", "Failed to initialize bus");
  }

#ifndef CONFIG_POCKET_TRS_TTGO_VGA32_SUPPORT
  // Configure SPI device for MCP23S17 and MCP23S08
  spi_mcp23x.address_bits = 0;
  spi_mcp23x.command_bits = 0;
  spi_mcp23x.dummy_bits = 0;
  spi_mcp23x.mode = 0;
  spi_mcp23x.duty_cycle_pos = 0;
  spi_mcp23x.cs_ena_posttrans = 0;
  spi_mcp23x.cs_ena_pretrans = 0;
  spi_mcp23x.clock_speed_hz = SPI_PORT_EXP_SPEED_MHZ * 1000 * 1000;
  spi_mcp23x.spics_io_num = SPI_PIN_NUM_CS_MCP23X;
  spi_mcp23x.flags = 0;
  spi_mcp23x.queue_size = 1;
  spi_mcp23x.pre_cb = NULL;
  spi_mcp23x.post_cb = NULL;
  ret = spi_bus_add_device(HSPI_HOST, &spi_mcp23x, &spi_mcp23x_h);
  ESP_ERROR_CHECK(ret);

  /*
   * The MCP23S17 and MCPS08 share the same CS line. By default, hardware addressing is disabled
   * for both chips (IOCON.HAEN == 0) and must be enabled. The first writePortExpander() command
   * is processed by both MCPs. However, MCP23S17_IOCONB is beyond the number of registers of the
   * MCP23S08 and will therefore be ignored by that chip. Once hardware addressing is enabled
   * for the MCP23S17, the same is done for the MCP23S08.
   */
  writePortExpander(MCP23S17, MCP23S17_IOCONB, MCP23S08_HAEN);
  writePortExpander(MCP23S08, MCP23S08_IOCON, MCP23S08_HAEN);

  // Configure ESP's MCP23S08 INT
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_DISABLE; //GPIO_INTR_NEGEDGE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1 << SPI_PIN_NUM_INT);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&io_conf);

#if 0
  gpio_install_isr_service(0);
  //hook isr handler for specific gpio pin
  gpio_isr_handler_add(SPI_PIN_NUM_INT, gpio_isr_handler, NULL);

  gpio_evt_queue = xQueueCreate(10, 0);
#endif

  // Configure SPI device for MCP4351
  spi_mcp4351.address_bits = 0;
  spi_mcp4351.command_bits = 0;
  spi_mcp4351.dummy_bits = 0;
  spi_mcp4351.mode = 0;
  spi_mcp4351.duty_cycle_pos = 0;
  spi_mcp4351.cs_ena_posttrans = 0;
  spi_mcp4351.cs_ena_pretrans = 0;
  spi_mcp4351.clock_speed_hz = SPI_DIGI_POT_SPEED_MHZ * 1000 * 1000;
  spi_mcp4351.spics_io_num = SPI_PIN_NUM_CS_MCP4351;
  spi_mcp4351.flags = 0;
  spi_mcp4351.queue_size = 1;
  spi_mcp4351.pre_cb = NULL;
  spi_mcp4351.post_cb = NULL;
  ret = spi_bus_add_device(HSPI_HOST, &spi_mcp4351, &spi_mcp4351_h);
  ESP_ERROR_CHECK(ret);

#ifdef CONFIG_POCKET_TRS_TEST_PCB
  test();
#endif

  /*
   * MCP23S17 configuration
   */
  // Port A is connected to D0-D7. Configure as input. Enable pull-ups.
  // Disable interrupts
  writePortExpander(MCP23S17, MCP23S17_IODIRA, 0xff);
  writePortExpander(MCP23S17, MCP23S17_GPPUA, 0xff);
  writePortExpander(MCP23S17, MCP23S17_GPINTENA, 0);
  // Port B is connected to A0-A7. Configure as output. Set 0 as initial address
  writePortExpander(MCP23S17, MCP23S17_IODIRB, 0);
  writePortExpander(MCP23S17, MCP23S17_GPIOB, 0);

  /*
   * MCP23S08 configuration
   */
  // Configure as input: TRS_IOBUSINT, TRS_IOBUSWAIT, TRS_EXTIOSEL
  writePortExpander(MCP23S08, MCP23S08_IODIR, TRS_IOBUSINT | TRS_IOBUSWAIT | TRS_EXTIOSEL);
  // Enable pull-ups
  writePortExpander(MCP23S08, MCP23S08_GPPU, 0xff);
  // Configure INT as open drain
  writePortExpander(MCP23S08, MCP23S08_IOCON, MCP23S08_ODR | MCP23S08_HAEN);
#if 0
  // Generate interrupt on pin change
  writePortExpander(MCP23S08, MCP23S08_INTCON, 0);
  // Enable interrupt-on-change
  writePortExpander(MCP23S08, MCP23S08_GPINTEN, 0xff);
  // Dummy read to clear INT
  readPortExpander(MCP23S08, MCP23S08_INTCAP);
#endif

#endif
}
