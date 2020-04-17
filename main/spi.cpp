
#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include "config.h"
#include "spi.h"

#define MCP23S_CHIP_ADDRESS 0x40
#define MCP23S_WRITE 0x00
#define MCP23S_READ  0x01

static spi_bus_config_t spi_bus;

static spi_device_interface_config_t spi_mcp23S17;
spi_device_handle_t spi_mcp23S17_h;

static spi_device_interface_config_t spi_mcp23S08;
spi_device_handle_t spi_mcp23S08_h;

static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
  xQueueSendFromISR(gpio_evt_queue, NULL, NULL);
}

void writeMCP(spi_device_handle_t dev, uint8_t cmd, uint8_t data)
{
  spi_transaction_t trans;

  memset(&trans, 0, sizeof(spi_transaction_t));
  trans.flags = SPI_TRANS_USE_TXDATA;
  trans.length = 3 * 8;   		// 3 bytes
  trans.tx_data[0] = MCP23S_CHIP_ADDRESS | MCP23S_WRITE;
  trans.tx_data[1] = cmd;
  trans.tx_data[2] = data;

  esp_err_t ret = spi_device_transmit(dev, &trans);
  ESP_ERROR_CHECK(ret);
}

uint8_t readMCP(spi_device_handle_t dev, uint8_t reg)
{
  spi_transaction_t trans;

  memset(&trans, 0, sizeof(spi_transaction_t));
  trans.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
  trans.length = 3 * 8;   		// 2 bytes
  trans.rxlength = 0;
  trans.tx_data[0] = MCP23S_CHIP_ADDRESS | MCP23S_READ;
  trans.tx_data[1] = reg;
  trans.tx_data[2] = 0;

  esp_err_t ret = spi_device_transmit(dev, &trans);
  ESP_ERROR_CHECK(ret);

  return trans.rx_data[2];
}

#if 1
void wire_test()
{
#if 0
  while (1) {
    if(xQueueReceive(gpio_evt_queue, NULL, portMAX_DELAY)) {
      uint8_t data = readMCP2(MCP23S08_INTCAP);
      Serial.println(data);
    }
  }
#else

#if 1
  // Writing
  uint8_t data = 0;
  writeMCP(MCP23S17, MCP23S17_IODIRA, 0);
  writeMCP(MCP23S17, MCP23S17_IODIRB, 0);
  writeMCP(MCP23S08, MCP23S08_IODIR, 0);

  while (1) {
    writeMCP(MCP23S17, MCP23S17_GPIOA, data);
    writeMCP(MCP23S17, MCP23S17_GPIOB, data);
    writeMCP(MCP23S08, MCP23S08_GPIO, data);
    delay(500);
    data ^= 0xff;
    writeMCP(MCP23S17, MCP23S17_GPIOA, data);
    writeMCP(MCP23S17, MCP23S17_GPIOB, data);
    writeMCP(MCP23S08, MCP23S08_GPIO, data);
    delay(500);
    data ^= 0xff;
  }
#else
  // reading
  uint8_t data = 0;
  writeMCP(MCP23S08, MCP23S08_IODIR, 0xff);
  writeMCP(MCP23S08, MCP23S08_GPPU, 0xff);

  while (1) {
    while (data == readMCP(MCP23S08, MCP23S08_GPIO)) ;
    data = readMCP(MCP23S08, MCP23S08_GPIO);
    Serial.println(data);
  }
#endif
#endif
}
#endif

static void test()
{
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_DISABLE; //GPIO_INTR_NEGEDGE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = GPIO_SEL_18;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);

  int cnt = 0;
  while(1) {
    printf("cnt: %d\n", cnt++);
    vTaskDelay(1000 / portTICK_RATE_MS);
    gpio_set_level(GPIO_NUM_18, cnt % 2);
    gpio_set_level(GPIO_NUM_18, cnt % 2);
  }
}

void init_spi()
{
  //test();
  // Configure SPI bus
  spi_bus.flags = SPICOMMON_BUSFLAG_MASTER;
  spi_bus.sclk_io_num = SPI_PIN_NUM_CLK;
  spi_bus.mosi_io_num = SPI_PIN_NUM_MOSI;
  spi_bus.miso_io_num = SPI_PIN_NUM_MISO;
  spi_bus.quadwp_io_num = -1;
  spi_bus.quadhd_io_num = -1;
  spi_bus.max_transfer_sz = 32;
  esp_err_t ret = spi_bus_initialize(SPI_HOST, &spi_bus, 0);
  ESP_ERROR_CHECK(ret);

  // Configure SPI device for MCP23S17
  spi_mcp23S17.address_bits = 0;
  spi_mcp23S17.command_bits = 0;
  spi_mcp23S17.dummy_bits = 0;
  spi_mcp23S17.mode = 0;
  spi_mcp23S17.duty_cycle_pos = 0;
  spi_mcp23S17.cs_ena_posttrans = 0;
  spi_mcp23S17.cs_ena_pretrans = 0;
  spi_mcp23S17.clock_speed_hz = SPI_SPEED_MHZ * 1000 * 1000;
  spi_mcp23S17.spics_io_num = SPI_PIN_NUM_CS_MCP23S17;
  spi_mcp23S17.flags = 0;
  spi_mcp23S17.queue_size = 1;
  spi_mcp23S17.pre_cb = NULL;
  spi_mcp23S17.post_cb = NULL;
  ret = spi_bus_add_device(SPI_HOST, &spi_mcp23S17, &spi_mcp23S17_h);
  ESP_ERROR_CHECK(ret);

  // Configure SPI device for MCP23S08
  spi_mcp23S08.address_bits = 0;
  spi_mcp23S08.command_bits = 0;
  spi_mcp23S08.dummy_bits = 0;
  spi_mcp23S08.mode = 0;
  spi_mcp23S08.duty_cycle_pos = 0;
  spi_mcp23S08.cs_ena_posttrans = 0;
  spi_mcp23S08.cs_ena_pretrans = 0;
  spi_mcp23S08.clock_speed_hz = SPI_SPEED_MHZ * 1000 * 1000;
  spi_mcp23S08.spics_io_num = SPI_PIN_NUM_CS_MCP23S08;
  spi_mcp23S08.flags = 0;
  spi_mcp23S08.queue_size = 1;
  spi_mcp23S08.pre_cb = NULL;
  spi_mcp23S08.post_cb = NULL;
  ret = spi_bus_add_device(HSPI_HOST, &spi_mcp23S08, &spi_mcp23S08_h);
  ESP_ERROR_CHECK(ret);

  // Configure ESP's MCP23S08 INT GPIO
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

  //wire_test();

  /*
   * MCP23S17 configuration
   */
  // Port B is connected to D0-D7. Configure as input. Disable pull-ups.
  // Disable interrupts
  writeMCP(MCP23S17, MCP23S17_IODIRB, 0xff);
  writeMCP(MCP23S17, MCP23S17_GPPUB, 0);
  writeMCP(MCP23S17, MCP23S17_GPINTENB, 0);
  // Port A is connected to A0-A7. Configure as output. Set 0 as initial address
  writeMCP(MCP23S17, MCP23S17_IODIRA, 0);
  writeMCP(MCP23S17, MCP23S17_GPIOA, 0);

  /*
   * MCP23S08 configuration
   */
  // Configure as input: TRS_IOBUSINT, TRS_IOBUSWAIT, TRS_EXTIOSEL
  writeMCP(MCP23S08, MCP23S08_IODIR, TRS_IOBUSINT | TRS_IOBUSWAIT | TRS_EXTIOSEL);
  // Enable pull-ups
  writeMCP(MCP23S08, MCP23S08_GPPU, 0xff);
  // Configure INT as open drain
  writeMCP(MCP23S08, MCP23S08_IOCON, MCP23S08_ODR);
#if 0
  // Generate interrupt on pin change
  writeMCP(MCP23S08, MCP23S08_INTCON, 0);
  // Enable interrupt-on-change
  writeMCP(MCP23S08, MCP23S08_GPINTEN, 0xff);
  // Dummy read to clear INT
  readMCP(MCP23S08, MCP23S08_INTCAP);
#endif
}
