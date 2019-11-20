
#include "fabgl.h"
#include "trs.h"
#include "sound.h"

#define AUDIO_U8 0
#define AUDIO_S16 1

#define DEFAULT_SAMPLE_RATE 16000
//44100

typedef unsigned char Uchar;
typedef unsigned char Uint8;
typedef unsigned short Uint16;
typedef Uint16 Ushort;
typedef unsigned int Uint32;
typedef unsigned long long tstate_t;

#define SOUND_RING_SIZE 16000

class TRSSamplesGenerator : public WaveformGenerator {
private:
  SemaphoreHandle_t xSemaphore = NULL;
  Uint8 sound_ring[SOUND_RING_SIZE];
  Uint8 *sound_ring_read_ptr = sound_ring;
  Uint8 *sound_ring_write_ptr = sound_ring;
  Uint8 *sound_ring_end = sound_ring + SOUND_RING_SIZE;

public:
  TRSSamplesGenerator();

  void setFrequency(int value);

  void putSample(Uchar sample);
  int getSample();
};

TRSSamplesGenerator::TRSSamplesGenerator()
{
  xSemaphore = xSemaphoreCreateMutex();
}


void TRSSamplesGenerator::setFrequency(int value)
{
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
  xSemaphoreTake(xSemaphore, portMAX_DELAY);
  if (sound_ring_read_ptr != sound_ring_write_ptr) {
    sample = *sound_ring_read_ptr++;
    if (sound_ring_read_ptr >= sound_ring_end) {
      sound_ring_read_ptr = sound_ring;
    }
  }
  xSemaphoreGive(xSemaphore);

  // process volume
  sample = sample * volume() / 127;

  return sample;
}

TRSSamplesGenerator trsSamplesGenerator;
SoundGenerator soundGenerator;

void init_sound() {
  soundGenerator.attach(&trsSamplesGenerator);
  trsSamplesGenerator.enable(true);
  soundGenerator.play(true);
}

#if 1

static int cassette_afmt = AUDIO_U8;
static float cassette_roundoff_error = 0.0;
static int cassette_value;
static int cassette_sample_rate = DEFAULT_SAMPLE_RATE;
static tstate_t cassette_transition;
static tstate_t last_sound;

const Uchar value_to_sample[] = { 127, /* 0.46 V */
          254, /* 0.85 V */
          0,   /* 0.00 V */
          127, /* unused, but close to 0.46 V */
};


void
transition_out(int value, tstate_t z80_state_t_count)
{
  Uchar sample;
  long nsamples, delta_us;
  Ushort code;
  float ddelta_us;

  //cassette_transitionsout++;
  //if (value != FLUSH && value == cassette_value) return;

  ddelta_us = (z80_state_t_count - cassette_transition) / CLOCK_MHZ_M3
    - cassette_roundoff_error;

//    if (cassette_state == SOUND) {
      if (ddelta_us > 20000.0) {
        /* Truncate silent periods */
        ddelta_us = 20000.0;
        cassette_roundoff_error = 0.0;
      }
      #if 0
      if (trs_event_scheduled() == transition_out ||
      trs_event_scheduled() == (trs_event_func) assert_state) {
        trs_cancel_event();
      }
      if (value == FLUSH) {
        trs_schedule_event((trs_event_func)assert_state, CLOSE, 5000000);
      } else {
        trs_schedule_event(transition_out, FLUSH,
                           (int)(25000 * z80_state.clockMHz));
      }
      #endif
 //   }
    sample = value_to_sample[cassette_value];
    nsamples = (unsigned long)
      (ddelta_us / (1000000.0/cassette_sample_rate) + 0.5);
    if (nsamples == 0) nsamples = 1; /* always at least one sample */
    cassette_roundoff_error =
      nsamples * (1000000.0/cassette_sample_rate) - ddelta_us;
#if CASSDEBUG
    debug("%d %4lu %d -> %3lu\n", cassette_value,
    z80_state.t_count - cassette_transition, value, nsamples);
#endif
    //if (cassette_format == DIRECT_FORMAT && cassette_stereo) nsamples *= 2;
    while (nsamples-- > 0) {
      trsSamplesGenerator.putSample(sample);
    }
    #if 0
    if (value == FLUSH) {
      value = cassette_value;
      trs_restore_delay();
    }
    #endif

  if (cassette_value != value) last_sound = z80_state_t_count;
  cassette_transition = z80_state_t_count;
  cassette_value = value;
}

#endif
