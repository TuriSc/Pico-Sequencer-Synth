/**
 * @file synth.c
 * @brief Implementation of the synth module.
 */
/*
 * Based on Pico Audio Pack Demo by Pimoroni
 * https://github.com/pimoroni/pimoroni-pico/tree/main/examples/pico_audio
 * Released under MIT license
 */

#include "pico/stdlib.h"
#include "synth.h"
#include <stdlib.h>

/**
 * @brief The audio channels.
 */
AudioChannel channels[CHANNEL_COUNT];

/**
 * @brief The value of pi.
 */
const float pi = 3.14159265358979323846f;

/**
 * @brief The state of the XOR shift PRNG.
 */
uint32_t prng_xorshift_state = 0x32B71700;

/**
 * @brief The sample rate of the audio output.
 */
uint32_t sample_rate = 44100;

/**
 * @brief Generates the next value in the XOR shift PRNG sequence.
 *
 * @return The next value in the sequence.
 */
uint32_t prng_xorshift_next() {
  uint32_t x = prng_xorshift_state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  prng_xorshift_state = x;
  return x;
}

/**
 * @brief Generates a random value with a rough approximation of a normal distribution.
 *
 * @return The generated value.
 */
int32_t prng_normal() {
  // rough approximation of a normal distribution
  uint32_t r0 = prng_xorshift_next();
  uint32_t r1 = prng_xorshift_next();
  uint32_t n = ((r0 & 0xffff) + (r1 & 0xffff) + (r0 >> 16) + (r1 >> 16)) / 2;
  return n - 0xffff;
}

/**
 * @brief The volume of the audio output.
 */
uint16_t volume = 0xffff;

/**
 * @brief The sine waveform sample.
 */
const int16_t sine_waveform[256] = {-32768,-32758,-32729,-32679,-32610,-32522,-32413,-32286,-32138,-31972,-31786,-31581,-31357,-31114,-30853,-30572,-30274,-29957,-29622,-29269,-28899,-28511,-28106,-27684,-27246,-26791,-26320,-25833,-25330,-24812,-24279,-23732,-23170,-22595,-22006,-21403,-20788,-20160,-19520,-18868,-18205,-17531,-16846,-16151,-15447,-14733,-14010,-13279,-12540,-11793,-11039,-10279,-9512,-8740,-7962,-7180,-6393,-5602,-4808,-4011,-3212,-2411,-1608,-804,0,804,1608,2411,3212,4011,4808,5602,6393,7180,7962,8740,9512,10279,11039,11793,12540,13279,14010,14733,15447,16151,16846,17531,18205,18868,19520,20160,20788,21403,22006,22595,23170,23732,24279,24812,25330,25833,26320,26791,27246,27684,28106,28511,28899,29269,29622,29957,30274,30572,30853,31114,31357,31581,31786,31972,32138,32286,32413,32522,32610,32679,32729,32758,32767,32758,32729,32679,32610,32522,32413,32286,32138,31972,31786,31581,31357,31114,30853,30572,30274,29957,29622,29269,28899,28511,28106,27684,27246,26791,26320,25833,25330,24812,24279,23732,23170,22595,22006,21403,20788,20160,19520,18868,18205,17531,16846,16151,15447,14733,14010,13279,12540,11793,11039,10279,9512,8740,7962,7180,6393,5602,4808,4011,3212,2411,1608,804,0,-804,-1608,-2411,-3212,-4011,-4808,-5602,-6393,-7180,-7962,-8740,-9512,-10279,-11039,-11793,-12540,-13279,-14010,-14733,-15447,-16151,-16846,-17531,-18205,-18868,-19520,-20160,-20788,-21403,-22006,-22595,-23170,-23732,-24279,-24812,-25330,-25833,-26320,-26791,-27246,-27684,-28106,-28511,-28899,-29269,-29622,-29957,-30274,-30572,-30853,-31114,-31357,-31581,-31786,-31972,-32138,-32286,-32413,-32522,-32610,-32679,-32729,-32758};

/**
 * @brief Checks if audio is currently playing.
 *
 * @return True if audio is playing, false otherwise.
 */
bool is_audio_playing() {
  if(volume == 0) {
    return false;
  }

  bool any_channel_playing = false;
  for(int c = 0; c < CHANNEL_COUNT; c++) {
    if(channels[c].volume > 0 && channels[c].adsr_phase != ADSR_OFF) {
      any_channel_playing = true;
    }
  }

  return any_channel_playing;
}

/**
 * @brief Generates a single audio frame.
 *
 * @return The generated audio frame.
 */
int16_t get_audio_frame() {
  int32_t sample = 0;  // used to combine channel output

  for(int c = 0; c < CHANNEL_COUNT; c++) {

    // increment the waveform position counter. this provides an
    // Q16 fixed point value representing how far through
    // the current waveform we are
    channels[c].waveform_offset += ((channels[c].frequency * 256) << 8) / sample_rate;

    if(channels[c].adsr_phase == ADSR_OFF) {
      continue;
    }

    if ((channels[c].adsr_frame >= channels[c].adsr_end_frame) && (channels[c].adsr_phase != SUSTAIN)) {
      switch (channels[c].adsr_phase) {
        case ATTACK:
        trigger_decay(&channels[c]);
        break;
      case DECAY:
        trigger_sustain(&channels[c]);
        break;
      case RELEASE:
        adsr_off(&channels[c]);
          break;
        default:
          break;
      }
    }

    channels[c].adsr += channels[c].adsr_step;
    channels[c].adsr_frame++;

    if(channels[c].waveform_offset & 0x10000) {
      // if the waveform offset overflows then generate a new
      // random noise sample
      channels[c].noise = prng_normal();
    }

    channels[c].waveform_offset &= 0xffff;

    // check if any waveforms are active for this channels[c]
    if(channels[c].waveforms) {
      uint8_t waveform_count = 0;
      int32_t channel_sample = 0;

      if(channels[c].waveforms & NOISE) {
        channel_sample += channels[c].noise;
        waveform_count++;
      }

      if(channels[c].waveforms & SAW) {
        channel_sample += (int32_t)channels[c].waveform_offset - 0x7fff;
        waveform_count++;
      }

      // creates a triangle wave of ^
      if (channels[c].waveforms & TRIANGLE) {
        if (channels[c].waveform_offset < 0x7fff) { // initial quarter up slope
          channel_sample += (int32_t)(channels[c].waveform_offset * 2) - (int32_t)0x7fff;
        }
        else { // final quarter up slope
          channel_sample += (int32_t)0x7fff - (((int32_t)channels[c].waveform_offset - (int32_t)0x7fff) * 2);
        }
        waveform_count++;
      }

      if (channels[c].waveforms & SQUARE) {
        channel_sample += (channels[c].waveform_offset < channels[c].pulse_width) ? 0x7fff : -0x7fff;
        waveform_count++;
      }

      if(channels[c].waveforms & SINE) {
        // the sine_waveform sample contains 256 samples in
        // total so we'll just use the most significant bits
        // of the current waveform position to index into it
        channel_sample += sine_waveform[channels[c].waveform_offset >> 8];
        waveform_count++;
      }

      if(channels[c].waveforms & WAVE) {
        channel_sample += channels[c].wave_buffer[channels[c].wave_buf_pos];
        if (++channels[c].wave_buf_pos == 64) {
          channels[c].wave_buf_pos = 0;
          channels[c].wave_buffer_callback(&channels[c]);
        }
        waveform_count++;
      }

      channel_sample = channel_sample / waveform_count;
      channel_sample = (int64_t)channel_sample * ((int32_t)(channels[c].adsr >> 8)) >> 16;

      // apply channel volume
      channel_sample = (int64_t)channel_sample * (int32_t)channels[c].volume >> 16;

      // apply channel filter
      //if (channel.filter_enable) {
        //float filter_epow = 1 - expf(-(1.0f / 22050.0f) * 2.0f * pi * (int32_t)(channel.filter_cutoff_frequency));
        //channel_sample += (channel_sample - channel.filter_last_sample) * filter_epow;
      //}

      //channel.filter_last_sample = channel_sample;

      // combine channel sample into the final sample
      sample += channel_sample;
    }
  }

  sample = (int64_t)sample * (int32_t)volume >> 16;

  // clip result to 16-bit
  sample = sample <= -0x8000 ? -0x8000 : (sample > 0x7fff ? 0x7fff : sample);
  return sample;
}

/**
 * @brief A no-op function for initializing the audio channel.
 *
 * @param channel The audio channel to initialize.
 */
static void noop(AudioChannel *channel){;}

/**
 * @brief Initializes the synth module.
 *
 * @param num_voices The number of voices to initialize.
 * @param _sample_rate The sample rate of the audio output.
 *
 * @return A pointer to the initialized audio channels.
 */
AudioChannel * synth_init(uint8_t num_voices, uint32_t _sample_rate) {
  sample_rate = _sample_rate;
  for(uint8_t i = 0; i < num_voices; i++) {
    channel_init(&channels[i]);
  }
  return channels;
}

/**
 * @brief Initializes an audio channel.
 *
 * @param channel The audio channel to initialize.
 */
static void channel_init(AudioChannel *channel) {
  channel->waveforms     = 0;      // bitmask for enabled waveforms
  channel->frequency     = 660;    // frequency of the voice (Hz)
  channel->volume        = 0xffff; // channel volume (default 50%)
  channel->attack_ms     = 2;      // attack period
  channel->decay_ms      = 6;      // decay period
  channel->sustain       = 0xffff; // sustain volume
  channel->release_ms    = 1;      // release period
  channel->pulse_width   = 0x7fff; // duty cycle of square wave (default 50%)
  channel->noise         = 0;      // current noise value
  channel->waveform_offset  = 0;   // voice offset (Q8)
  channel->filter_last_sample = 0;
  channel->filter_enable = false;
  channel->filter_cutoff_frequency = 0;
  channel->adsr_frame    = 0;      // number of frames into the current ADSR phase
  channel->adsr_end_frame = 0;     // frame target at which the ADSR changes to the next phase
  channel->adsr          = 0;
  channel->adsr_step     = 0;
  channel->adsr_phase    = ADSR_OFF;
  channel->wave_buf_pos  = 0;      //
  channel->wave_buffer[64];        // buffer for arbitrary waveforms. small as it's filled by user callback
  channel->user_data     = NULL;
  channel->wave_buffer_callback = noop;
};

/**
 * @brief Triggers the attack phase of the ADSR envelope.
 *
 * @param channel The audio channel to trigger the attack phase for.
 */
void trigger_attack(AudioChannel *channel)  {
  channel->adsr_frame = 0;
  channel->adsr_phase = ATTACK;
  channel->adsr_end_frame = (channel->attack_ms * sample_rate) / 1000;
  channel->adsr_step = (int32_t)(0xffffff - (int32_t)channel->adsr) / (int32_t)channel->adsr_end_frame;
}

/**
 * @brief Triggers the decay phase of the ADSR envelope.
 *
 * @param channel The audio channel to trigger the decay phase for.
 */
void trigger_decay(AudioChannel *channel) {
  channel->adsr_frame = 0;
  channel->adsr_phase = DECAY;
  channel->adsr_end_frame = (channel->decay_ms * sample_rate) / 1000;
  channel->adsr_step = (int32_t)((channel->sustain << 8) - (int32_t)channel->adsr) / (int32_t)channel->adsr_end_frame;
}

/**
 * @brief Triggers the sustain phase of the ADSR envelope.
 *
 * @param channel The audio channel to trigger the sustain phase for.
 */
void trigger_sustain(AudioChannel *channel) {
  channel->adsr_frame = 0;
  channel->adsr_phase = SUSTAIN;
  channel->adsr_end_frame = 0;
  channel->adsr_step = 0;
}

/**
 * @brief Triggers the release phase of the ADSR envelope.
 *
 * @param channel The audio channel to trigger the release phase for.
 */
void trigger_release(AudioChannel *channel) {
  channel->adsr_frame = 0;
  channel->adsr_phase = RELEASE;
  channel->adsr_end_frame = (channel->release_ms * sample_rate) / 1000;
  channel->adsr_step = (int32_t)(0 - (int32_t)channel->adsr) / (int32_t)channel->adsr_end_frame;
}

/**
 * @brief Turns off the ADSR envelope.
 *
 * @param channel The audio channel to turn off the ADSR envelope for.
 */
void adsr_off(AudioChannel *channel) {
  channel->adsr_frame = 0;
  channel->adsr_phase = ADSR_OFF;
  channel->adsr_step = 0;
}

/**
 * @brief Sets the volume of the audio output.
 *
 * @param percent The volume as a percentage (0-100).
 */
void set_volume(uint8_t percent) {
   // Clamp the input value to the range of 0-100
    if (percent <= 0) {
        volume = 0;
        return;
    } else if (percent > 100) {
        volume = 0xFFFF;
        return;
    }
    volume = percent * 0xFFFF / 100;
}

/**
 * @brief Sets the sample rate of the audio output.
 *
 * @param _sample_rate The sample rate to set.
 */
void set_sample_rate(uint32_t _sample_rate) {
    sample_rate = _sample_rate;
}

