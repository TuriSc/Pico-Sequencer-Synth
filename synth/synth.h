#ifndef SYNTH_H
#define SYNTH_H

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

  // The duration a note is played is determined by the amount of attack,
  // decay, and release, combined with the length of the note as defined by
  // the user.
  //
  // - Attack:  number of milliseconds it takes for a note to hit full volume
  // - Decay:   number of milliseconds it takes for a note to settle to sustain volume
  // - Sustain: percentage of full volume that the note sustains at (duration implied by other factors)
  // - Release: number of milliseconds it takes for a note to reduce to zero volume after it has ended
  //
  // Attack (750ms) - Decay (500ms) -------- Sustain ----- Release (250ms)
  //
  //                +         +                                  +    +
  //                |         |                                  |    |
  //                |         |                                  |    |
  //                |         |                                  |    |
  //                v         v                                  v    v
  // 0ms               1000ms              2000ms              3000ms              4000ms
  //
  // |              XXXX |                   |                   |                   |
  // |             X    X|XX                 |                   |                   |
  // |            X      |  XXX              |                   |                   |
  // |           X       |     XXXXXXXXXXXXXX|XXXXXXXXXXXXXXXXXXX|                   |
  // |          X        |                   |                   |X                  |
  // |         X         |                   |                   |X                  |
  // |        X          |                   |                   | X                 |
  // |       X           |                   |                   | X                 |
  // |      X            |                   |                   |  X                |
  // |     X             |                   |                   |  X                |
  // |    X              |                   |                   |   X               |
  // |   X               |                   |                   |   X               |
  // |  X +    +    +    |    +    +    +    |    +    +    +    |    +    +    +    |    +
  // | X  |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
  // |X   |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
  // +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+--->

  #define CHANNEL_COUNT 8 // Number of maximum simultaneous voices

  enum Waveform {
    NOISE     = 128,
    SQUARE    = 64,
    SAW       = 32,
    TRIANGLE  = 16,
    SINE      = 8,
    WAVE      = 1
  };

  enum ADSRPhase {
    ATTACK,
    DECAY,
    SUSTAIN,
    RELEASE,
    ADSR_OFF
  };

  typedef struct AudioChannel {
  uint8_t   waveforms;      // bitmask for enabled waveforms
  uint16_t  frequency;    // frequency of the voice (Hz)
  uint16_t  volume; // channel volume (default 50%)

  uint16_t  attack_ms;      // attack period
  uint16_t  decay_ms;      // decay period
  uint16_t  sustain; // sustain volume
  uint16_t  release_ms;      // release period
  uint16_t  pulse_width; // duty cycle of square wave (default 50%)
  int16_t   noise;      // current noise value

  uint32_t  waveform_offset;   // voice offset (Q8)

  int32_t   filter_last_sample;
  bool      filter_enable;
  uint16_t  filter_cutoff_frequency;

  uint32_t  adsr_frame;      // number of frames into the current ADSR phase
  uint32_t  adsr_end_frame;     // frame target at which the ADSR changes to the next phase
  uint32_t  adsr;
  int32_t   adsr_step;
  enum      ADSRPhase adsr_phase;

  uint8_t   wave_buf_pos;      //
  int16_t   wave_buffer[64];        // buffer for arbitrary waveforms. small as it's filled by user callback

  void      *user_data;
  void      (*wave_buffer_callback)(struct AudioChannel *channel);

} AudioChannel;


AudioChannel * synth_init(uint8_t num_voices, uint32_t _sample_rate);
static void channel_init(AudioChannel *channel);
void trigger_attack(AudioChannel *channel);
void trigger_decay(AudioChannel *channel);
void trigger_sustain(AudioChannel *channel);
void trigger_release(AudioChannel *channel);
void adsr_off(AudioChannel *channel);

int16_t get_audio_frame();
bool is_audio_playing();

void set_volume(uint8_t percent);
void set_sample_rate(uint32_t _sample_rate);

#ifdef __cplusplus
}
#endif

#endif
