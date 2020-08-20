
#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/i2s.h>
#include <driver/adc.h>


//#define NOISE_FLOOR 1024
//#define SIGNAL_CENTER 2048
#define NOISE_FLOOR 64
#define SIGNAL_CENTER 127

#define I2S_SAMPLING_FREQ 22000
#define RING_BUFFER_SIZE 6000
#define DMA_BUFFER_SIZE 512

#define SPEED_500     0
#define SPEED_1500    1
#define SPEED_250     2

typedef unsigned char Uchar;
typedef unsigned char Uint8;
typedef unsigned short Uint16;
typedef Uint16 Ushort;
typedef unsigned int Uint32;


#define SOUND_RING_SIZE 16000

class TRSSamplesGenerator {
private:
  SemaphoreHandle_t xSemaphore = NULL;
  Uint8 sound_ring[SOUND_RING_SIZE];
  Uint8 *sound_ring_read_ptr = sound_ring;
  Uint8 *sound_ring_write_ptr = sound_ring;
  Uint8 *sound_ring_end = sound_ring + SOUND_RING_SIZE;

public:
  TRSSamplesGenerator();

  void putSample(Uchar sample);
  int getSample();
};

extern TRSSamplesGenerator* trsSamplesGenerator;

void init_i2s();
uint8_t getSample();
