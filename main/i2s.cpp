
#include "i2s.h"

#define DMA_BUFFER_SIZE_IN_BYTES (DMA_BUFFER_SIZE * sizeof(uint16_t))

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


static void i2sWrite(uint16_t* buf, int buf_size)
{
  while (!cassette_motor) {

    int mainVolume = 100;

    for (int i = 0; i < DMA_BUFFER_SIZE; ++i) {
      int sample = 0, tvol = 0;
      sample = trsSamplesGenerator->getSample();
      int avol = tvol ? imin(127, 127 * 127 / tvol) : 127;
      sample = sample * avol / 127;
      sample = sample * mainVolume / 127;

      buf[i + (i & 1 ? -1 : 1)] = (127 + sample) << 8;
    }

    size_t bytes_written;
    i2s_write(I2S_NUM_0, buf, DMA_BUFFER_SIZE_IN_BYTES, &bytes_written, portMAX_DELAY);
  }
}

static portMUX_TYPE DRAM_ATTR mux = portMUX_INITIALIZER_UNLOCKED;

static volatile uint8_t ring_buffer[RING_BUFFER_SIZE];
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
    vTaskDelay(35 / portTICK_RATE_MS);
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

static void i2sRead(uint16_t* buf, int buf_size)
{
  ESP_ERROR_CHECK(i2s_adc_enable(I2S_NUM_0));

  while (cassette_motor) {
    size_t br;
    ESP_ERROR_CHECK(i2s_read(I2S_NUM_0, (void*) buf, buf_size, &br, portMAX_DELAY));
    for(int i = 0; i < br / 2; i++) {
      uint16_t sample = buf[i];
      sample >>= 4;
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
  ESP_ERROR_CHECK(i2s_adc_disable(I2S_NUM_0));
}

static void determine_signal_center(uint16_t* buf, int buf_size)
{
  int center_cnt = 0;
  size_t br;

  ESP_ERROR_CHECK(i2s_adc_enable(I2S_NUM_0));

  while (center_cnt < I2S_SAMPLING_FREQ) {
    size_t br;
    ESP_ERROR_CHECK(i2s_read(I2S_NUM_0, (void*) buf, buf_size, &br, portMAX_DELAY));
    for(int i = 0; i < br / 2; i++) {
      uint16_t sample = buf[i];
      sample >>= 4;
      signal_center = (signal_center * 99 + sample) / 100.0;
      center_cnt++;
    }
  }

  ESP_ERROR_CHECK(i2s_adc_disable(I2S_NUM_0));
  printf("Signal center: %f\n", signal_center);
}

static void i2sTask(void * arg)
{
  uint16_t* buf = (uint16_t*) malloc(DMA_BUFFER_SIZE_IN_BYTES);
  determine_signal_center(buf, DMA_BUFFER_SIZE_IN_BYTES);

  while (true) {
    i2sRead(buf, DMA_BUFFER_SIZE_IN_BYTES);
    i2sWrite(buf, DMA_BUFFER_SIZE_IN_BYTES);
  }
}

void init_i2s()
{
  esp_err_t ret;
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX |
			  I2S_MODE_ADC_BUILT_IN | I2S_MODE_DAC_BUILT_IN),
    .sample_rate          = I2S_SAMPLING_FREQ,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags     = 0,
    .dma_buf_count        = 2,
    .dma_buf_len          = DMA_BUFFER_SIZE * sizeof(uint16_t),
    .use_apll             = false,
    .tx_desc_auto_clear   = 0,
    .fixed_mclk           = 0
  };
  
  ret = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  ESP_ERROR_CHECK(ret);
  //  ret = i2s_set_pin(I2S_NUM_0, NULL);
  //ESP_ERROR_CHECK(ret);
  ret = i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_0);
  ESP_ERROR_CHECK(ret);
  i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN); // GPIO25

  i2s_set_clk(I2S_NUM_0, I2S_SAMPLING_FREQ,
	      I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);

  ret = i2s_start(I2S_NUM_0);
  ESP_ERROR_CHECK(ret);

  //adc1_config_width(ADC_WIDTH_BIT_12);
  //adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);

  trsSamplesGenerator = new TRSSamplesGenerator();
  xTaskCreatePinnedToCore(i2sTask, "i2s", 3000, NULL,
			  tskIDLE_PRIORITY + 3, NULL, 1);
}
