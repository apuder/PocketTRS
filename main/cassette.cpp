
#define CASSDEBUG 0
#define CASSDEBUG2 0
#define CASSDEBUG3 0
#define CASSDEBUG4 0

#include <string.h>
#include <stdlib.h>
#include <Arduino.h>
#include <driver/adc.h>
#include <driver/i2s.h>
#include <soc/sens_reg.h>
#include <soc/sens_struct.h>
#include "soc/rtc_wdt.h"

#include "cassette.h"

typedef uint32_t Uint32;
typedef uint16_t Ushort;
typedef uint32_t Uint;
typedef uint8_t Uchar;


#define FALSE 0

#define I2S_SAMPLING_FREQ 16000
#define RING_BUFFER_SIZE 12000
#define DEFAULT_SAMPLE_RATE 16000
#define SAMPLE_BUFFER_SIZE 512
#define DMA_BUFFER_SIZE 512

#define CLOSE   0
#define READ    1
#define WRITE   2
#define SOUND           3  /* used for OSS_SOUND only */
#define ORCH90          4  /* used for OSS_SOUND only */
#define FAILED          5

#define CAS_FORMAT         1  /* recovered bit/byte stream */
#define CPT_FORMAT         2  /* cassette pulse train w/ exact timing */
#define WAV_FORMAT         3  /* wave file */
#define DIRECT_FORMAT      4  /* direct to sound card */
#define DEBUG_FORMAT       5  /* like cpt but in ASCII */
static char *format_name[] = {
  NULL, "cas", "cpt", "wav", "direct", "debug" };
//#define NOISE_FLOOR 1024
//#define SIGNAL_CENTER 2048
#define NOISE_FLOOR 64
#define SIGNAL_CENTER 127

#define DEFAULT_FORMAT    CAS_FORMAT

#define FLUSH -500  /* special fake signal value used when turning off motor */

static int cassette_position = 0;
static unsigned int cassette_format = DEFAULT_FORMAT;
static int cassette_state = CLOSE;
static int cassette_motor = 1; // XXX
static float cassette_avg;
static float cassette_env;
static int cassette_noisefloor;
static int cassette_sample_rate = DEFAULT_SAMPLE_RATE;
int cassette_default_sample_rate = DEFAULT_SAMPLE_RATE;
static int cassette_stereo = 0;
static Uint32 cassette_silence;
static int soundDeviceOpen = FALSE;

int trs_sound = 1;

/* For bit-level emulation */
static tstate_t cassette_transition;
static tstate_t last_sound;
static tstate_t cassette_firstoutread;
static int cassette_value, cassette_next, cassette_flipflop;
static int cassette_lastnonzero;
static int cassette_transitionsout;
static unsigned long cassette_delta;
static float cassette_roundoff_error = 0.0;

/* For bit/byte conversion (.cas file i/o) */
static int cassette_byte;
static int cassette_bitnumber;
static int cassette_pulsestate;
#define SPEED_500     0
#define SPEED_1500    1
#define SPEED_250     2
int cassette_speed = SPEED_500;

/* Pulse shapes for conversion from .cas on input */
#define CAS_MAXSTATES 8
struct {
  int delta_us;
  int next;
} pulse_shape[3][2][CAS_MAXSTATES] = {
  {{
    /* Low-speed zero: clock 1 data 0 */
    { 0,    1 },
    { 128,  2 },
    { 128,  0 },
    { 1871, 0 },             /* normally 1757; 1871 after 8th bit */
    { -1,  -1 }
  }, {
      /* Low-speed one: clock 1 data 1 */
      { 0,    1 },
      { 128,  2 },
      { 128,  0 },
      { 748,  1 },
      { 128,  2 },
      { 128,  0 },
      { 860, 0 },                   /* normally 748; 860 after 8th bit; 1894 after a5 sync */
      { -1,  -1 }
    }}, {{
    /* High-speed zero: wide pulse */
    { 0,    1 },
    { 376,  2 },
    { 376,  1 },
    { -1,  -1 }
  }, {
      /* High-speed one: narrow pulse */
      { 0,    1 },
      { 188,  2 },
      { 188,  1 },
      { -1,  -1 }
    }}, {{
    /* Level I zero: clock 1 data 0 */
    { 0,    1 },
    { 125,  2 },
    { 125,  0 },
    { 3568, 0 },
    { -1,  -1 }
  }, {
      /* Level I one: clock 1 data 1 */
      { 0,    1 },
      { 128,  2 },
      { 128,  0 },
      { 1673, 1 },
      { 128,  2 },
      { 128,  0 },
      { 1673, 0 },
      { -1,  -1 }
    }}
};

/* States and thresholds for conversion to .cas on output */
#define ST_INITIAL   0
#define ST_500GOTCLK 1
#define ST_500GOTDAT 2
#define ST_1500      3
#define ST_250       4
#define ST_250GOTCLK 5
#define ST_250GOTDAT 6
#define ST_500THRESH  1250.0 /* us threshold between 0 and 1 */
#define ST_1500THRESH  282.0 /* us threshold between 1 and 0 */
#define ST_250THRESH  2500.0 /* us threshold between 0 and 1 */

#define DETECT_250    1200.0 /* detect level 1 input routine */

/* Values for conversion to .wav on output */
/* Values in comments are from Model I technical manual.  Model III/4 are
   close though not quite the same, as one resistor in the network was
   changed; we ignore the difference.  Actually, we ignore more than
   that; we convert the values as if 0 were really halfway between
   high and low.  */
Uchar value_to_sample[] = { 127, /* 0.46 V */
                            254, /* 0.85 V */
                            0, /* 0.00 V */
                            127, /* unused, but close to 0.46 V */
};

/* .wav file definitions */
#define WAVE_FORMAT_PCM (0x0001)
#define WAVE_FORMAT_MONO 1
#define WAVE_FORMAT_STEREO 2
#define WAVE_FORMAT_8BIT 8
#define WAVE_FORMAT_16BIT 16
#define WAVE_RIFFSIZE_OFFSET 0x04
#define WAVE_RIFF_OFFSET 0x08
#define WAVE_DATAID_OFFSET 0x24
#define WAVE_DATASIZE_OFFSET 0x28
#define WAVE_DATA_OFFSET 0x2c
static long wave_dataid_offset = WAVE_DATAID_OFFSET;
static long wave_datasize_offset = WAVE_DATASIZE_OFFSET;
static long wave_data_offset = WAVE_DATA_OFFSET;


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
    static int i = 0;
    i++;
    if (i == 1000) {
      //printf("#########\n");
      i = 0;
    }
    ring_buffer_read_ptr++;
    if (ring_buffer_read_ptr >= ring_buffer_end) {
      ring_buffer_read_ptr = ring_buffer;
    }
  }
  portEXIT_CRITICAL_ISR(&mux);
}

static uint8_t getSample() {
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
  } else {
    printf("----\n");
  }
  portEXIT_CRITICAL(&mux);
  return sample;
}

static void i2sReadTask(void* param)
{
  static uint16_t buf[1024];
  static float center = SIGNAL_CENTER;
  static int center_cnt = 0;
#if 0
  int64_t now = esp_timer_get_time();
  size_t r, total;
  total = 0;
  for (int x = 0; x < 1000; x++) {
    esp_err_t ret = i2s_read(I2S_NUM_0, (void*) buf, sizeof(buf), &r, portMAX_DELAY);
    ESP_ERROR_CHECK(ret);
    total += r;
  }
  printf("I2S reading (%u): %lld\n", total, esp_timer_get_time() - now);
#endif
  
  while (true) {
    size_t br;
    esp_err_t ret = i2s_read(I2S_NUM_0, (void*) buf, sizeof(buf), &br, portMAX_DELAY);
    ESP_ERROR_CHECK(ret);
    //printf("Writing %d samples\n", br  /2);
    //portENTER_CRITICAL_ISR(&mux);
    for(int i = 0; i < br / 2; i++) {
      uint16_t sample = buf[i];
      sample >>= 4;
#if 1
      if (center_cnt < DEFAULT_SAMPLE_RATE) {
	center = (center * 99 + sample) / 100.0;
	center_cnt++;
	if (center_cnt == DEFAULT_SAMPLE_RATE) {
	  printf("Signal center: %f\n", center);
	}
      }
#else
      center = SIGNAL_CENTER;
#endif
      if (sample < (center - cassette_noisefloor)) {
	putSample(1);
	//printf(".........\n");
      } else if (sample > (center + cassette_noisefloor)) {
	putSample(2);
	//printf("--------------------------\n");
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
	int cabs = abs(sample - center);
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
    //portEXIT_CRITICAL(&mux);
  }
}


uint8_t getSamplex() {
  static uint16_t ring_buffer[RING_BUFFER_SIZE];
  static int current_idx = 0;
  static size_t last_idx = 0;

  if (current_idx == last_idx) {
    esp_err_t ret = i2s_read(I2S_NUM_0, (void*) ring_buffer, RING_BUFFER_SIZE * sizeof(uint16_t), &last_idx, portMAX_DELAY);
    ESP_ERROR_CHECK(ret);
    printf("%d\n", last_idx);
    last_idx >>= 1;
    current_idx = 0;
  }
  uint16_t sample = ring_buffer[current_idx++];
  if (sample < 1500) {
    return 1;
  } else if (sample > 2800) {
    return 2;
  } else {
    return 0;
  }
}


/* Return value: 1 = already that state; 0 = state changed; -1 = failed */
int assert_state(int state)
{
  if (cassette_state == state) {
    return 1;
  }
  cassette_state = state;
  return 0;
}


/* Read a new transition, updating cassette_next and cassette_delta.
   If file read fails (perhaps due to eof), return 0, else 1.
   Set cassette_delta to (unsigned long) -1 on failure. */
static int
transition_in()
{
  unsigned long delta_us, nsamples;
  long maxsamples;
  Ushort code;
  Uint d;
  int next, ret = 0;
  int c, cabs;
  float delta_ts;

  nsamples = 0;
  maxsamples = cassette_sample_rate / 100;
  do {
    int direct = (cassette_format == DIRECT_FORMAT);
    next = getSample();
  #if 0
    if (c > 127 + cassette_noisefloor) {
      next = 1;
    } else if (c <= 127 - cassette_noisefloor) {
      next = 2;
    } else {
      next = 0;
    }
    if (cassette_speed == SPEED_1500) {
      cassette_noisefloor = 2;
    } else {
      /* Attempt to learn the correct noise cutoff adaptively.
       * This code is just a hack; it would be nice to know a
       * real signal-processing algorithm for this application
       */
      cabs = abs(c - 127);
#if CASSDEBUG2
      debug("%f %f %d %d -> %d\n", cassette_avg, cassette_env,
            cassette_noisefloor, cabs, next);
#endif
      if (cabs > 1) {
        cassette_avg = (99*cassette_avg + cabs)/100;
      }
      if (cabs > cassette_env) {
        cassette_env = (cassette_env + 9*cabs)/10;
      } else if (cabs > 10) {
        cassette_env = (99*cassette_env + cabs)/100;
      }
      cassette_noisefloor = (cassette_avg + cassette_env)/2;
    }
#endif
    nsamples++;
    /* Allow reset button */
  #if 0
    trs_get_event(0);
    if (z80_state.nmi) break;
  #endif
  } while (next == cassette_value && maxsamples-- > 0);
  cassette_next = next;
  delta_ts = nsamples * (1000000.0/cassette_sample_rate)
             * CLOCK_MHZ_M3 - cassette_roundoff_error;
  cassette_delta = (unsigned long) delta_ts + 0.5;
  cassette_roundoff_error = cassette_delta - delta_ts;
#if CASSDEBUG
  debug("%3lu -> %d %4lu %d\n",
        nsamples, cassette_value, cassette_delta, cassette_next);
#endif
  ret = 1;
//fail:
  if (ret == 0) {
    cassette_delta = (unsigned long) -1;
  }
  return ret;
}

/* Z80 program is turning motor on or off */
void trs_cassette_motor(int value, tstate_t z80_state_t_count)
{
  if (value) {
    /* motor on */
    if (!cassette_motor) {
#if CASSDEBUG3
      debug("motor on %ld\n", z80_state_t_count);
#endif
      cassette_motor = 1;
      cassette_transition = z80_state_t_count;
      cassette_value = 0;
      cassette_next = 0;
      cassette_delta = 0;
      cassette_flipflop = 0;
      cassette_byte = 0;
      cassette_bitnumber = 0;
      cassette_pulsestate = 0;
      cassette_speed = SPEED_500;
      cassette_roundoff_error = 0.0;
      cassette_avg = NOISE_FLOOR;
      cassette_env = SIGNAL_CENTER;
      cassette_noisefloor = NOISE_FLOOR;
      cassette_firstoutread = 0;
      cassette_transitionsout = 0;
#if 0
      if (trs_model > 1) {
	/* Get 1500bps reading started after 1 second */
	trs_schedule_event(trs_cassette_kickoff, 0,
			   (tstate_t) (1000000 * z80_state.clockMHz));
      }
#endif
    }
  } else {
    /* motor off */
    if (cassette_motor) {
#if 0
      if (cassette_state == WRITE) {
        transition_out(FLUSH);
      }
#endif
      assert_state(CLOSE);
      cassette_motor = 0;
    }
  }
}

void
trs_cassette_update(tstate_t z80_state_t_count)
{
  if (cassette_motor && cassette_state != WRITE && assert_state(READ) >= 0) {
    int newtrans = 0;
    //printf(".................\n");
    while ((z80_state_t_count - cassette_transition) >= cassette_delta) {
      //printf("@@@@@@@@@@ %llu, %llu, %lu\n", z80_state_t_count, cassette_transition, cassette_delta);
      /* Simulate analog signal processing on the 500-bps cassette input */
      if (cassette_next != 0 && cassette_value == 0) {
        cassette_flipflop = 0x80;
      }

      /* Deliver the previously read transition from the file */
      cassette_value = cassette_next;
      cassette_transition += cassette_delta;

      /* Remember last nonzero value to get hysteresis in 1500 bps
         zero-crossing detector */
      if (cassette_value != 0) cassette_lastnonzero = cassette_value;

      /* Read the next transition */
      newtrans = transition_in();

#if 0
      /* Allow reset button */
      trs_get_event(0);
      if (z80_state.nmi) return;
#endif
    }
    /* Schedule an interrupt on the 1500-bps cassette input if needed */
  #if 0
    if (newtrans && cassette_speed == SPEED_1500) {
      if (cassette_next == 2 && cassette_lastnonzero != 2) {
        trs_schedule_event(trs_cassette_fall_interrupt, 1,
                           cassette_delta -
                           (z80_state_t_count - cassette_transition));
      } else if (cassette_next == 1 && cassette_lastnonzero != 1) {
        trs_schedule_event(trs_cassette_rise_interrupt, 1,
                           cassette_delta -
                           (z80_state_t_count - cassette_transition));
      } else {
        trs_schedule_event(trs_cassette_update, 0,
                           cassette_delta -
                           (z80_state_t_count - cassette_transition));
      }
    }
#endif
  }
}

void trs_cassette_out(int value, tstate_t z80_state_t_count)
{
#if CASSDEBUG3
  debug("out %ld %d %d %d\n", z80_state.t_count, value, cassette_motor, cassette_state);
#endif
  if (cassette_motor) {
    if (cassette_state == READ) {
      trs_cassette_update(z80_state_t_count);
      cassette_flipflop = 0;
      if (cassette_firstoutread == 0) {
	cassette_firstoutread = z80_state_t_count;
      }
    }
#if 0
    if (cassette_state != READ && value != cassette_value) {
      if (assert_state(WRITE) < 0) return;
      transition_out(value);
    }
#endif
  }

#if 0
  /* Do sound emulation by sending samples to /dev/dsp */
  if (trs_sound && cassette_motor == 0) {
    if (cassette_state != SOUND && value == 0) return;
    if (assert_state(SOUND) < 0) return;
#ifdef SUSPEND_DELAY
    trs_suspend_delay();
#endif
    transition_out(value);
  }
#endif
}

int
trs_cassette_in(tstate_t z80_state_t_count)
{
#if CASSDEBUG3
  debug("in  %ld\n", z80_state_t_count);
#endif
  if (cassette_motor && cassette_transitionsout <= 1) {
    assert_state(READ);
  }
  /* Heuristic to detect reading with Level 1 routines.  If the
     routine paused too long after resetting the flipflop before
     reading it again, assume it must be Level 1 code.  */
  if (cassette_firstoutread > 1) {
    if ((z80_state_t_count - cassette_firstoutread)
        / CLOCK_MHZ_M3 > DETECT_250) {
      cassette_speed = SPEED_250;
    } else {
      cassette_speed = SPEED_500;
    }
#if CASSDEBUG4
    debug("250 detector = %s (%f)\n",
          (cassette_speed == SPEED_250) ? "yes" : "no",
          (z80_state_t_count - cassette_firstoutread) / CLOCK_MHZ_M3);
#endif
    cassette_firstoutread = 1;             /* disable detector */
  }
  //trs_cassette_clear_interrupts();
  trs_cassette_update(z80_state_t_count);
#if 0
  if (trs_model == 1) {
    return cassette_flipflop;
  } else {
#endif
  return cassette_flipflop | (cassette_lastnonzero == 1);
#if 0
}
#endif
}

void
trs_cassette_reset()
{
  assert_state(CLOSE);
}

void init_cassette_in()
{
#if 1
  esp_err_t ret;
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
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
#else
  i2s_config_t i2s_config;
  esp_err_t ret;

  i2s_config.mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN);
  i2s_config.sample_rate          = I2S_SAMPLING_FREQ;
  i2s_config.bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT;
  i2s_config.communication_format = I2S_COMM_FORMAT_I2S_MSB;
  i2s_config.channel_format       = I2S_CHANNEL_FMT_ONLY_RIGHT;
  i2s_config.intr_alloc_flags     = 0;
  i2s_config.dma_buf_count        = 2;
  i2s_config.dma_buf_len          = DMA_BUFFER_SIZE * sizeof(uint16_t);
  i2s_config.use_apll             = false;
  i2s_config.tx_desc_auto_clear   = 0;
#endif
  
  ret = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  ESP_ERROR_CHECK(ret);
  //  ret = i2s_set_pin(I2S_NUM_0, NULL);
  //ESP_ERROR_CHECK(ret);
  ret = i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_0);
  ESP_ERROR_CHECK(ret);
  ret = i2s_start(I2S_NUM_0);
  ESP_ERROR_CHECK(ret);

  //adc1_config_width(ADC_WIDTH_BIT_12);
  //adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);

  ret = i2s_adc_enable(I2S_NUM_0);
  ESP_ERROR_CHECK(ret);

#if 1
  xTaskCreatePinnedToCore(i2sReadTask, "cass", 2048, NULL,
                          tskIDLE_PRIORITY + 3, NULL, 0);

#else
  uint16_t* i2s_read_buff = (uint16_t*) calloc(SAMPLE_BUFFER_SIZE, sizeof(uint16_t));
  assert(i2s_read_buff != NULL);
  size_t bytes_read;
  while (true) {
    //read data from I2S bus, in this case, from ADC.
    ret = i2s_read(I2S_NUM_0, (void*) i2s_read_buff, SAMPLE_BUFFER_SIZE * sizeof(uint16_t), &bytes_read, portMAX_DELAY);
    ESP_ERROR_CHECK(ret);
#if 1
    printf("%d\n", i2s_read_buff[0]);
#else
    for (int i = 0; i < bytes_read / 2; i++) {
      printf("%5d ", i2s_read_buff[i]);
      if (i % 8 == 0) printf("\n");
    }
#endif
  }
  i2s_adc_disable(I2S_NUM_0);
  free(i2s_read_buff);
  i2s_read_buff = NULL;
#endif
}
