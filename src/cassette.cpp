
#define CASSDEBUG 0
#define CASSDEBUG2 0
#define CASSDEBUG3 0
#define CASSDEBUG4 0

#include <string.h>
#include <stdlib.h>
#include <Arduino.h>
#include <driver/adc.h>
#include <soc/sens_reg.h>
#include <soc/sens_struct.h>
#include "soc/rtc_wdt.h"

#include "cassette.h"

typedef uint32_t Uint32;
typedef uint16_t Ushort;
typedef uint32_t Uint;
typedef uint8_t Uchar;


#define FALSE 0

#define DEFAULT_SAMPLE_RATE 1000

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
#define NOISE_FLOOR 64

#define DEFAULT_FORMAT    CAS_FORMAT

#define FLUSH -500  /* special fake signal value used when turning off motor */

static int cassette_position = 0;
static unsigned int cassette_format = DEFAULT_FORMAT;
static int cassette_state = CLOSE;
static int cassette_motor = 0;
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


#define RING_BUFFER_SIZE 4000

static portMUX_TYPE DRAM_ATTR mux = portMUX_INITIALIZER_UNLOCKED;

static hw_timer_t* adcTimer = NULL;

static uint8_t ring_buffer[RING_BUFFER_SIZE];
static uint8_t *ring_buffer_read_ptr = ring_buffer;
static uint8_t *ring_buffer_write_ptr = ring_buffer;
static uint8_t *ring_buffer_end = ring_buffer + RING_BUFFER_SIZE;

void IRAM_ATTR putSample(uint8_t sample) {
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
  if (ring_buffer_read_ptr != ring_buffer_write_ptr) {
    sample = *ring_buffer_read_ptr++;
    if (ring_buffer_read_ptr >= ring_buffer_end) {
      ring_buffer_read_ptr = ring_buffer;
    }
  }
  portEXIT_CRITICAL(&mux);
  return sample;
}

int IRAM_ATTR local_adc1_read(int channel) {
    uint16_t adc_value;
    SENS.sar_meas_start1.sar1_en_pad = (1 << channel); // only one channel is selected
    while (SENS.sar_slave_addr1.meas_status != 0);
    SENS.sar_meas_start1.meas1_start_sar = 0;
    SENS.sar_meas_start1.meas1_start_sar = 1;
    while (SENS.sar_meas_start1.meas1_done_sar == 0);
    adc_value = SENS.sar_meas_start1.meas1_data_sar;
    return adc_value;
}

void IRAM_ATTR handleInterrupt() {
  int sample = adc1_get_raw(ADC1_CHANNEL_0);//local_adc1_read(ADC1_CHANNEL_0);
  if (sample < 1200) {
    putSample(2);
  } else if (sample > 2400) {
    putSample(1);
  } else {
    putSample(0);
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
  unsigned long delta_us, nsamples, maxsamples;
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
    if (direct && cassette_stereo) {
      /* Discard right channel */
      (void) get_sample(direct, cassette_file);
    }
  #endif
    //if (c == EOF) goto fail;
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
fail:
  if (ret == 0) {
    cassette_delta = (unsigned long) -1;
  }
  return ret;
}

void
trs_cassette_update(tstate_t z80_state_t_count)
{
  if (cassette_motor && cassette_state != WRITE && assert_state(READ) >= 0) {
    int newtrans = 0;
    while ((z80_state_t_count - cassette_transition) >= cassette_delta) {

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

      /* Allow reset button */
#if 0
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
  trs_cassette_update(0);
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

static void cassette_task(void* param)
{
#if 1

#if 1
  adcTimer = timerBegin(0, 80, true);
  timerAttachInterrupt(adcTimer, handleInterrupt, true);
  timerAlarmWrite(adcTimer, 1000000 / DEFAULT_SAMPLE_RATE, true);
  timerAlarmEnable(adcTimer);
#endif

#else
  while(1) {
    unsigned long ts = micros() + (1000000 / DEFAULT_SAMPLE_RATE);
    handleInterrupt();
    rtc_wdt_feed();
    unsigned long sleepTime = ts - micros();
    //Serial.println(sleepTime);
    delayMicroseconds(sleepTime);
  }
#endif
  vTaskDelete(NULL);
}

void init_cassette_in()
{
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
#if 1

#if 1
  rtc_wdt_protect_off();
  rtc_wdt_disable();
  xTaskCreatePinnedToCore(cassette_task, "cass", 1024, NULL,
                          tskIDLE_PRIORITY, NULL, 0);
#else
  adcTimer = timerBegin(0, 80, true);
  timerAttachInterrupt(adcTimer, handleInterrupt, true);
  timerAlarmWrite(adcTimer, 1000000 / DEFAULT_SAMPLE_RATE, true);
  timerAlarmEnable(adcTimer);
#endif
#else
  int min = 1024;
  int max = 0;
  int avg = 0;
  while (1) {
    int val = adc1_get_raw(ADC1_CHANNEL_0);
    printf("%5d\n", val);
    delay(10);
  }
#endif
}
