/*
  Created by Fabrizio Di Vittorio (fdivitto2013@gmail.com) - <http://www.fabgl.com>
  Copyright (c) 2019-2020 Fabrizio Di Vittorio.
  All rights reserved.

  This file is part of FabGL Library.

  FabGL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  FabGL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with FabGL.  If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once


/**
 * @file
 *
 * @brief This file contains all classes related to FabGL Sound System
 */




#include <stdint.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"

#include "fabglconf.h"
#include "fabutils.h"



namespace fabgl {


#define DEFAULT_SAMPLE_RATE 16000

// 512 samples, at 16KHz generate a send every 512/16000*1000 = 32ms (16000/512=31.25 sends per second)
// 200 samples, at 16Khz generate a send every 200/16000*1000 = 12.5ms (16000/200=80 sends per second)
#define I2S_SAMPLE_BUFFER_SIZE 200  // must be even

#define WAVEGENTASK_STACK_SIZE 1024


/** @brief Base abstract class for waveform generators. A waveform generator can be seen as an audio channel that will be mixed by SoundGenerator. */
class WaveformGenerator {
public:
  WaveformGenerator() : next(nullptr), m_sampleRate(0), m_volume(100), m_enabled(false), m_duration(-1), m_autoDestroy(false), m_autoDetach(false) { }

  virtual ~WaveformGenerator() { }

  /**
   * @brief Sets output frequency
   *
   * @param value Frequency in Hertz
   */
  virtual void setFrequency(int value) = 0;

  /**
   * @brief Sets number of samples to play
   *
   * @param value Number of samples to play. -1 = infinite.
   */
  void setDuration(uint32_t value) { m_duration = value; }

  /**
   * @brief Returns number of remaining samples to play
   *
   * @return Number of remaining samples to play. -1 = infinite.
   */
  uint32_t duration() { return m_duration; }

  /**
   * @brief Sets autodetach mode
   *
   * @value if true: this object needs to be detached from the sound generator when there are no more samples to play.
   */
  void setAutoDetach(bool value) { m_autoDetach = value; }

  bool autoDetach() { return m_autoDetach; }

  /**
   * @brief Sets autodestroy mode
   *
   * @value if true: this object needs to be destroyed by the sound generat or when there are no more samples to play. This will set also setAutoDetach(true).
   */
  void setAutoDestroy(bool value) { m_autoDestroy = value; m_autoDetach |= value; }

  bool autoDestroy() { return m_autoDestroy; }

  /**
   * @brief Gets next sample
   *
   * @return Sample value as signed 8 bit (-128..127 range)
   */
  virtual int getSample() = 0;

  /**
   * @brief Sets volume of this generator
   *
   * @param value Volume value. Minimum is 0, maximum is 127.
   */
  void setVolume(int value) { m_volume = value; }

  /**
   * @brief Determines current volume
   *
   * @return Current volume of this generator (0 = minimum, 127 = maximum)
   */
  int volume() { return m_volume; }

  /**
   * @brief Determines whether this generator is enabled or disabled
   *
   * @return True if this generator is enabled
   */
  bool enabled() { return m_enabled; }

  /**
   * @brief Enables or disabled this generator
   *
   * A generator is disabled for default and must be enabled in order to play sound
   *
   * @param value True to enable the generator, False to disable
   */
  void enable(bool value) { m_enabled = value; }

  /**
   * @brief Sets the sample rate
   *
   * Default sample rate is 160000 Hertz.
   *
   * @param value Sample rate in Hertz
   */
  void setSampleRate(int value) { m_sampleRate = value; }

  /**
   * @brief Determines the sample rate
   *
   * @return Current sample rate in Hertz
   */
  uint16_t sampleRate() { return m_sampleRate; }

  WaveformGenerator * next;

protected:

  void decDuration() { --m_duration; if (m_duration == 0) m_enabled = false; }

private:
  uint16_t m_sampleRate;
  int8_t   m_volume;
  int8_t   m_enabled;   // 0 = disabled, 1 = enabled
  uint32_t m_duration;  // number of samples to play (-1 = infinite)
  bool     m_autoDestroy; // if true: this object needs to be destroyed by the sound generator when there are no more samples to play
  bool     m_autoDetach;  // if true: this object needs to be autodetached from the sound generator when there are no more samples to play
};


/** @brief Sine waveform generator */
class SineWaveformGenerator : public WaveformGenerator {
public:
  SineWaveformGenerator();

  void setFrequency(int value);

  int getSample();

private:
  uint32_t m_phaseInc;
  uint32_t m_phaseAcc;
  uint16_t m_frequency;
  int16_t  m_lastSample;
};


/** @brief Square waveform generator */
class SquareWaveformGenerator : public WaveformGenerator {
public:
  SquareWaveformGenerator();

  void setFrequency(int value);

  /**
   * @brief Sets square wave duty cycle
   *
   * @param dutyCycle Duty cycle in 0..255 range. 255 = 100%
   */
  void setDutyCycle(int dutyCycle);

  int getSample();

private:
  uint32_t m_phaseInc;
  uint32_t m_phaseAcc;
  uint16_t m_frequency;
  int8_t   m_lastSample;
  uint8_t  m_dutyCycle;
};


/** @brief Triangle waveform generator */
class TriangleWaveformGenerator : public WaveformGenerator {
public:
  TriangleWaveformGenerator();

  void setFrequency(int value);

  int getSample();

private:
  uint32_t m_phaseInc;
  uint32_t m_phaseAcc;
  uint16_t m_frequency;
  int8_t   m_lastSample;
};


/** @brief Sawtooth waveform generator */
class SawtoothWaveformGenerator : public WaveformGenerator {
public:
  SawtoothWaveformGenerator();

  void setFrequency(int value);

  int getSample();

private:
  uint32_t m_phaseInc;
  uint32_t m_phaseAcc;
  uint16_t m_frequency;
  int8_t   m_lastSample;
};


/** @brief Noise generator */
class NoiseWaveformGenerator : public WaveformGenerator {
public:
  NoiseWaveformGenerator();

  void setFrequency(int value);

  int getSample();

private:
  uint16_t m_noise;
};


/////////////////////////////////////////////////////////////////////////////////////////////
// "tries" to emulate VIC6561 noise generator
// derived from a reverse enginnered VHDL code: http://www.denial.shamani.dk/bb/viewtopic.php?t=8733&start=210

/**
 * @brief Emulates VIC6561 (VIC20) noise generator
 *
 * Inspired from a reverse enginnered VHDL code: http://www.denial.shamani.dk/bb/viewtopic.php?t=8733&start=210
 */
class VICNoiseGenerator : public WaveformGenerator {
public:
  VICNoiseGenerator();

  void setFrequency(int value);
  uint16_t frequency() { return m_frequency; }

  int getSample();

private:
  static const uint16_t LFSRINIT = 0x0202;
  static const int      CLK      = 4433618;

  uint16_t m_frequency;
  uint16_t m_counter;
  uint16_t m_LFSR;
  uint16_t m_outSR;
};



/**
 * @brief Samples generator
 *
 * Sample data should be sampled at the same samplerate of the sound generator.
 * Only 8 bit (signed - not compressed) depth is supported.
 */
class SamplesGenerator : public WaveformGenerator {
public:
  SamplesGenerator(int8_t const * data, int length);

  void setFrequency(int value);

  int getSample();

private:
  int8_t const * m_data;
  int            m_length;
  int            m_index;
};


/**
 * @brief SoundGenerator handles audio output
 *
 * Applications attach waveform generators (like SineWaveformGenerator, SquareWaveformGenerator, etc...) and call SoundGenerator.play() to start audio generation.
 *
 * The GPIO used for audio output is GPIO-25. See @ref confAudio "Configuring Audio port" for audio connection sample schema.
 *
 * Here is a list of supported sound generators:
 * - SineWaveformGenerator
 * - SquareWaveformGenerator
 * - TriangleWaveformGenerator
 * - SawtoothWaveformGenerator
 * - NoiseWaveformGenerator
 * - SamplesGenerator
 */
class SoundGenerator {

public:

  /**
   * @brief Creates an instance of the sound generator. Only one instance is allowed
   */
  SoundGenerator(int sampleRate = DEFAULT_SAMPLE_RATE);

  ~SoundGenerator();

  /**
   * @brief Stops playing and removes all attached waveform generators
   */
  void clear();

  /**
   * @brief Starts or stops playing
   *
   * @param value True = starts playing, False = stops playing
   *
   * @return Returns previous playing state
   *
   * Example:
   *
   *     soundGenerator.play(true);
   */
  bool play(bool value);

  /**
   * @brief Plays the specified samples
   *
   * Starts immediately to play the specified samples. It is not required to call play().
   * This method returns without wait the end of sound.
   *
   * @param data Samples to play.
   * @param length Number of samples to play.
   * @param volume Volume value. Minimum is 0, maximum is 127.
   * @param durationMS Duration in milliseconds. 0 = untile end of samples, -1 = infinite loop.
   *
   * @return Pointer to SamplesGenerator object. Lifetime of this object is limited to the duration.
   */
  SamplesGenerator * playSamples(int8_t const * data, int length, int volume = 100, int durationMS = 0);

  /**
   * @brief Plays the specified waveform
   *
   * Starts immediately to play the specified waveform. It is not required to call play().
   * This method returns without wait the end of sound.
   *
   * @param waveform Waveform to play.
   * @param frequency Frequency in Hertz.
   * @param durationMS Duration in milliseconds.
   * @param volume Volume value. Minimum is 0, maximum is 127.
   *
   * Example:
   *
   *     // plays a sinewave at 500Hz for 200 milliseconds
   *     soundGen.playSound(SineWaveformGenerator(), 500, 200);
   *
   *     // plays a C Major chord for 1 second
   *     soundGen.playSound(SineWaveformGenerator(), 262, 1000);  // C
   *     soundGen.playSound(SineWaveformGenerator(), 330, 1000);  // E
   *     soundGen.playSound(SineWaveformGenerator(), 392, 1000);  // G
   */
  template <typename T>
  void playSound(T const & waveform, int frequency, int durationMS, int volume = 100) {
    auto wf = new T(waveform);
    attach(wf);
    wf->setFrequency(frequency);
    wf->setVolume(volume);
    wf->setAutoDestroy(true);
    wf->setDuration(m_sampleRate / 1000 * durationMS);
    wf->enable(true);
    play(true);
  }

  /**
   * @brief Determines whether sound generator is playing
   *
   * @return True when playing, False otherwise
   */
  bool playing();

  WaveformGenerator * channels() { return m_channels; }

  /**
   * @brief Attaches a waveform generator
   *
   * @param value Pointer of the waveform generator to attach
   *
   * Example:
   *
   *     SineWaveformGenerator sine;
   *     soundGenerator.attach(&sine);
   *     sine.enable(true);
   *     sine.setFrequency(500);  // 500 Hz
   */
  void attach(WaveformGenerator * value);

  /**
   * @brief Detaches a waveform generator
   *
   * @param value Pointer of the waveform generator to detach
   */
  void detach(WaveformGenerator * value);

  /**
   * @brief Sets the overall volume
   *
   * @param value Volume value. Minimum is 0, maximum is 127.
   */
  void setVolume(int value) { m_volume = value; }

  /**
   * @brief Determines current overall volume
   *
   * @return Current overall volume (0 = minimum, 127 = maximum)
   */
  int volume() { return m_volume; }


private:

  void i2s_audio_init();
  static void waveGenTask(void * arg);
  bool suspendPlay(bool value);
  void mutizeOutput();
  void detachNoSuspend(WaveformGenerator * value);


  TaskHandle_t        m_waveGenTaskHandle;

  WaveformGenerator * m_channels;

  uint16_t *          m_sampleBuffer;

  int8_t              m_volume;

  uint16_t            m_sampleRate;

};




} // end of namespace

