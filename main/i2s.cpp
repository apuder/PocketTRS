
#include "i2s.h"
#include "esp_attr.h"
#include "esp_adc/adc_continuous.h"
#include "driver/dac_continuous.h"
#include <stdlib.h>

/*
 * Cassette input (ADC1 channel 0, GPIO36) and sound output (DAC channel 0,
 * GPIO25). On the ESP32 the continuous ADC and DAC drivers both run on
 * I2S0, so only one of them exists at a time: the ADC while the cassette
 * motor is on, the DAC otherwise.
 */

// One ADC conversion result (adc_digi_output_data_t) is 2 bytes
#define ADC_BUFFER_SIZE_IN_BYTES (DMA_BUFFER_SIZE * sizeof(uint16_t))

#define imin(a, b) ((a) > (b)) ? (b) : (a)

typedef unsigned long long tstate_t;

extern int cassette_state;
extern float cassette_avg;
extern float cassette_env;
extern float signal_center;
extern int cassette_noisefloor;
extern int cassette_speed;
extern int cassette_motor;

#define SOUND           3  /* used for OSS_SOUND only */


TRSSamplesGenerator::TRSSamplesGenerator()
{
  xSemaphore = xSemaphoreCreateMutex();
}


void TRSSamplesGenerator::putSample(Uchar sample) {
  xSemaphoreTake(xSemaphore, portMAX_DELAY);
  *sound_ring_write_ptr++ = sample;
  if (sound_ring_write_ptr >= sound_ring_end) {
    sound_ring_write_ptr = sound_ring;
  }
  if (sound_ring_write_ptr == sound_ring_read_ptr) {
    sound_ring_read_ptr++;
    if (sound_ring_read_ptr >= sound_ring_end) {
      sound_ring_read_ptr = sound_ring;
    }
  }
  xSemaphoreGive(xSemaphore);
}

int TRSSamplesGenerator::getSample() {
  int sample = 0;
  int volume = 100;
  
  xSemaphoreTake(xSemaphore, portMAX_DELAY);
  if (sound_ring_read_ptr != sound_ring_write_ptr) {
    sample = *sound_ring_read_ptr++;
    if (sound_ring_read_ptr >= sound_ring_end) {
      sound_ring_read_ptr = sound_ring;
    }
  }
  xSemaphoreGive(xSemaphore);

  // process volume
  sample = sample * volume / 127;

  return sample;
}

TRSSamplesGenerator* trsSamplesGenerator = NULL;


static void i2sWrite(uint8_t* buf)
{
  dac_continuous_handle_t dac;
  dac_continuous_config_t dac_config = {
    .chan_mask = DAC_CHANNEL_MASK_CH0, // GPIO25
    .desc_num = 4,
    .buf_size = DMA_BUFFER_SIZE * sizeof(uint16_t),
    .freq_hz = I2S_SAMPLING_FREQ,
    .offset = 0,
    .clk_src = DAC_DIGI_CLK_SRC_DEFAULT,
    .chan_mode = DAC_CHANNEL_MODE_SIMUL
  };
  ESP_ERROR_CHECK(dac_continuous_new_channels(&dac_config, &dac));
  ESP_ERROR_CHECK(dac_continuous_enable(dac));

  while (!cassette_motor) {

    int mainVolume = 100;

    for (int i = 0; i < DMA_BUFFER_SIZE; ++i) {
      int sample = 0, tvol = 0;
      sample = trsSamplesGenerator->getSample();
      int avol = tvol ? imin(127, 127 * 127 / tvol) : 127;
      sample = sample * avol / 127;
      sample = sample * mainVolume / 127;

      buf[i] = 127 + sample;
    }

    // The driver puts each 8 bit sample into the 16 bit slot I2S0 sends
    ESP_ERROR_CHECK(dac_continuous_write(dac, buf, DMA_BUFFER_SIZE, NULL, -1));
  }

  ESP_ERROR_CHECK(dac_continuous_disable(dac));
  ESP_ERROR_CHECK(dac_continuous_del_channels(dac));
}

static portMUX_TYPE DRAM_ATTR mux = portMUX_INITIALIZER_UNLOCKED;

// In PSRAM: only tasks read and write it
static volatile uint8_t ring_buffer[RING_BUFFER_SIZE] EXT_RAM_BSS_ATTR;
static volatile uint8_t *ring_buffer_read_ptr = ring_buffer;
static volatile uint8_t *ring_buffer_write_ptr = ring_buffer;
static volatile uint8_t *ring_buffer_end = ring_buffer + RING_BUFFER_SIZE;

static void putSample(uint8_t sample) {
  portENTER_CRITICAL_ISR(&mux);
  *ring_buffer_write_ptr++ = sample;
  if (ring_buffer_write_ptr >= ring_buffer_end) {
    ring_buffer_write_ptr = ring_buffer;
  }
  if (ring_buffer_write_ptr == ring_buffer_read_ptr) {
    ring_buffer_read_ptr++;
    if (ring_buffer_read_ptr >= ring_buffer_end) {
      ring_buffer_read_ptr = ring_buffer;
    }
  }
  portEXIT_CRITICAL_ISR(&mux);
}

uint8_t getSample() {
  uint8_t sample = 0;
  portENTER_CRITICAL(&mux);
#if 1
  if (ring_buffer_read_ptr == ring_buffer_write_ptr) {
    portEXIT_CRITICAL(&mux);
    vTaskDelay(35 / portTICK_PERIOD_MS);
    portENTER_CRITICAL(&mux);
  }
#endif
  if (ring_buffer_read_ptr != ring_buffer_write_ptr) {
    sample = *ring_buffer_read_ptr++;
    if (ring_buffer_read_ptr >= ring_buffer_end) {
      ring_buffer_read_ptr = ring_buffer;
    }
  }
  portEXIT_CRITICAL(&mux);
  return sample;
}

static adc_continuous_handle_t adc_start()
{
  adc_continuous_handle_t adc;
  adc_continuous_handle_cfg_t handle_config = {
    .max_store_buf_size = 4 * ADC_BUFFER_SIZE_IN_BYTES,
    .conv_frame_size = ADC_BUFFER_SIZE_IN_BYTES,
  };
  ESP_ERROR_CHECK(adc_continuous_new_handle(&handle_config, &adc));

  adc_digi_pattern_config_t pattern = {
    .atten = ADC_ATTEN_DB_12,
    .channel = ADC_CHANNEL_0, // GPIO36
    .unit = ADC_UNIT_1,
    .bit_width = ADC_BITWIDTH_12
  };
  adc_continuous_config_t adc_config = {
    .pattern_num = 1,
    .adc_pattern = &pattern,
    .sample_freq_hz = I2S_SAMPLING_FREQ,
    .conv_mode = ADC_CONV_SINGLE_UNIT_1
  };
  ESP_ERROR_CHECK(adc_continuous_config(adc, &adc_config));
  ESP_ERROR_CHECK(adc_continuous_start(adc));
  return adc;
}

static void adc_stop(adc_continuous_handle_t adc)
{
  ESP_ERROR_CHECK(adc_continuous_stop(adc));
  ESP_ERROR_CHECK(adc_continuous_deinit(adc));
}

// Reads conversion results; returns how many. Each is 12 bits of data
// below the channel number.
static uint32_t adc_read(adc_continuous_handle_t adc, adc_digi_output_data_t* buf)
{
  uint32_t br = 0;
  esp_err_t err = adc_continuous_read(adc, (uint8_t*) buf, ADC_BUFFER_SIZE_IN_BYTES, &br, 100);
  if (err == ESP_ERR_TIMEOUT) {
    return 0;
  }
  ESP_ERROR_CHECK(err);
  return br / sizeof(adc_digi_output_data_t);
}

static void i2sRead(adc_digi_output_data_t* buf)
{
  adc_continuous_handle_t adc = adc_start();

  while (cassette_motor) {
    uint32_t n = adc_read(adc, buf);
    for(int i = 0; i < n; i++) {
      // 8 bit samples
      uint16_t sample = buf[i].type1.data >> 4;
      if (sample < (signal_center - cassette_noisefloor)) {
	putSample(1);
      } else if (sample > (signal_center + cassette_noisefloor)) {
	putSample(2);
      } else {
	putSample(0);
      }
      if (cassette_speed == SPEED_1500) {
	cassette_noisefloor = 2;
      } else {
	/* Attempt to learn the correct noise cutoff adaptively.
	 * This code is just a hack; it would be nice to know a
	 * real signal-processing algorithm for this application
	 */
	//	int cabs = abs(sample - (4096 / 2));
	int cabs = abs(sample - signal_center);
#if CASSDEBUG2
	debug("%f %f %d %d -> %d\n", cassette_avg, cassette_env,
	       cassette_noisefloor, cabs, next);
#endif
	if (cabs > 1) {
	  cassette_avg = (99*cassette_avg + cabs) / 100;
	}
	if (cabs > cassette_env) {
	  cassette_env = (cassette_env + 9*cabs) / 10;
	} else if (cabs > 10) {
	  cassette_env = (99*cassette_env + cabs) / 100;
	}
	cassette_noisefloor = (cassette_avg + cassette_env) / 2;
      }
    }
  }

  adc_stop(adc);
}

static void determine_signal_center(adc_digi_output_data_t* buf)
{
  int center_cnt = 0;
  adc_continuous_handle_t adc = adc_start();

  while (center_cnt < I2S_SAMPLING_FREQ) {
    uint32_t n = adc_read(adc, buf);
    for(int i = 0; i < n; i++) {
      uint16_t sample = buf[i].type1.data >> 4;
      signal_center = (signal_center * 99 + sample) / 100.0;
      center_cnt++;
    }
  }

  adc_stop(adc);
  printf("Signal center: %f\n", signal_center);
}

static void i2sTask(void * arg)
{
  // Used for ADC results as well as for DAC samples
  void* buf = malloc(ADC_BUFFER_SIZE_IN_BYTES);
  determine_signal_center((adc_digi_output_data_t*) buf);

  while (true) {
    i2sRead((adc_digi_output_data_t*) buf);
    i2sWrite((uint8_t*) buf);
  }
}

void init_i2s()
{
  trsSamplesGenerator = new TRSSamplesGenerator();
  xTaskCreatePinnedToCore(i2sTask, "i2s", 3000, NULL,
			  tskIDLE_PRIORITY + 3, NULL, 1);
}
