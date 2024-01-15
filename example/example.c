/* Pico Sequencer Synth example
** A polyphonic, multitimbral DDS (Direct Digital Synthesis) library for
** Raspberry Pi Pico, equipped with a sequencer supporting up to 8 channels.
**
** By Turi Scandurra
** https://turiscandurra.com/circuits
**
** Based on a C++ demo by Pimoroni
** https://github.com/pimoroni/pimoroni-pico/tree/main/examples/pico_audio
**
** I2S driver based on the work of Ricardo Massaro
** https://github.com/moefh/
**/

#include "pico/stdlib.h"
#include <synth_sequencer.h>

#define SAMPLE_RATE 22050
// 44100 Hz is the default sample rate, but it breaks
// a little with complex sequences (more than 3 voices)

#if USE_AUDIO_PWM
  #define PWM_AUDIO_PIN           0
#elif USE_AUDIO_I2S
  #define I2S_DATA_PIN             28 // -> I2S DIN
  #define I2S_CLOCK_PIN_BASE       26 // -> I2S BCK
  // The third required connection is GPIO 27 -> I2S LRCK (BCK+1)

  static const struct sound_i2s_config sound_config = {
    .pin_sda         = I2S_DATA_PIN,
    .pin_scl         = I2S_CLOCK_PIN_BASE,
    .pin_ws          = I2S_CLOCK_PIN_BASE + 1,
    .sample_rate     = SAMPLE_RATE,
    .bits_per_sample = 16,
    .pio_num         = 0, // 0 for pio0, 1 for pio1
  };

#endif

#define NUM_VOICES  5
#define NUM_NOTES 128
#define KICK      500
#define HH      20000

const int16_t notes[NUM_VOICES][NUM_NOTES] = {
  { // Arp
    AS3, -1,  D4, -1,  F4, -1, AS4, -1, AS3, -1,  D4, -1,  F4, -1, AS4, -1,
    AS3, -1,  D4, -1,  F4, -1, AS4, -1, AS3, -1,  D4, -1,  F4, -1, AS4, -1,
     G3, -1, AS3, -1,  D4, -1,  F4, -1,  G3, -1, AS3, -1,  D4, -1,  F4, -1,
     G3, -1, AS3, -1,  D4, -1,  F4, -1,  G3, -1, AS3, -1,  D4, -1,  F4, -1,
     A3, -1,  C4, -1,  D4, -1,  A4, -1,  A3, -1,  C4, -1,  D4, -1,  A4, -1,
     A3, -1,  C4, -1,  D4, -1,  A4, -1,  A3, -1,  C4, -1,  D4, -1,  A4, -1,
     G3, -1, AS3, -1,  C4, -1,  D4, -1,  G3, -1, AS3, -1,  C4, -1,  D4, -1,
     G3, -1, AS3, -1,  C4, -1,  D4, -1,  G3, -1, AS3, -1,  C4, -1,  D4, -1,
  },
  { // Pad
     F3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    AS2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
     C3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
     D3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  },
  { // Bass
    AS2, 0, -1, 0, AS3, 0, -1, AS2, 0, AS2, 0, AS2, AS3, 0, -1, 0,
    AS2, 0, -1, 0, AS3, 0, -1, AS2, 0, AS2, 0, AS2, AS3, 0, -1, 0,
    DS2, 0, -1, 0, DS3, 0, -1, DS2, 0, DS2, 0, DS2, DS3, 0, -1, 0,
    DS2, 0, -1, 0, DS3, 0, -1, DS2, 0, DS2, 0, DS2, DS3, 0, -1, 0,
     F2, 0, -1, 0,  F3, 0, -1,  F2, 0,  F2, 0,  F2,  F3, 0, -1, 0,
     F2, 0, -1, 0,  F3, 0, -1,  F2, 0,  F2, 0,  F2,  F3, 0, -1, 0,
     G2, 0, -1, 0,  G3, 0, -1,  G2, 0,  G2, 0,  G2,  G3, 0, -1, 0,
     G2, 0, -1, 0,  G3, 0, -1,  G2, 0,  G2, 0,  G2,  G3, 0, -1, 0,
  },
  { // Kick drum
    KICK, -1, 0, 0, KICK, -1, 0, 0, KICK, -1, 0, 0, KICK, -1, 0, 0,
    KICK, -1, 0, 0, KICK, -1, 0, 0, KICK, -1, 0, 0, KICK, -1, 0, 0,
    KICK, -1, 0, 0, KICK, -1, 0, 0, KICK, -1, 0, 0, KICK, -1, 0, 0,
    KICK, -1, 0, 0, KICK, -1, 0, 0, KICK, -1, 0, 0, KICK, -1, 0, 0,
    KICK, -1, 0, 0, KICK, -1, 0, 0, KICK, -1, 0, 0, KICK, -1, 0, 0,
    KICK, -1, 0, 0, KICK, -1, 0, 0, KICK, -1, 0, 0, KICK, -1, 0, 0,
    KICK, -1, 0, 0, KICK, -1, 0, 0, KICK, -1, 0, 0, KICK, -1, 0, 0,
    KICK, -1, 0, 0, KICK, -1, 0, 0, KICK, -1, 0, 0, KICK, -1, 0, 0,
  },
  { // Hi-hat
    0, 0, HH, -1, 0, 0, HH, -1, 0, 0, HH, -1, HH, 0, HH, -1,
    0, 0, HH, -1, 0, 0, HH, -1, 0, 0, HH, -1, HH, 0, HH, -1,
    0, 0, HH, -1, 0, 0, HH, -1, 0, 0, HH, -1, HH, 0, HH, -1,
    0, 0, HH, -1, 0, 0, HH, -1, 0, 0, HH, -1, HH, 0, HH, -1,
    0, 0, HH, -1, 0, 0, HH, -1, 0, 0, HH, -1, HH, 0, HH, -1,
    0, 0, HH, -1, 0, 0, HH, -1, 0, 0, HH, -1, HH, 0, HH, -1,
    0, 0, HH, -1, 0, 0, HH, -1, 0, 0, HH, -1, HH, 0, HH, -1,
    0, 0, HH, -1, 0, 0, HH, -1, 0, 0, HH, -1, HH, 0, HH, -1,
  },
};

int main() {
  stdio_init_all();
  AudioChannel * voices = synth_init(NUM_VOICES, SAMPLE_RATE);

  #if USE_AUDIO_PWM
    sound_pwm_init(PWM_AUDIO_PIN, SAMPLE_RATE);
  #elif USE_AUDIO_I2S
    sound_i2s_init(&sound_config);
  #endif

  // Initialize voices
  sequencer_init(NUM_VOICES, (const int16_t *)notes, NUM_NOTES);

  // Configure voices
  // Arp
  voices[0].waveforms   = TRIANGLE | SQUARE;
  voices[0].attack_ms   = 16;
  voices[0].decay_ms    = 168;
  voices[0].sustain     = 0xafff;
  voices[0].release_ms  = 168;
  voices[0].volume      = 10000;

  // Pad
  voices[1].waveforms   = SINE | SQUARE;
  voices[1].attack_ms   = 56;
  voices[1].decay_ms    = 2000;
  voices[1].sustain     = 0;
  voices[1].release_ms  = 0x8080;
  voices[1].volume      = 10000;

  // Bass
  voices[2].waveforms   = SQUARE;
  voices[2].attack_ms   = 10;
  voices[2].decay_ms    = 100;
  voices[2].sustain     = 0;
  voices[2].release_ms  = 500;
  voices[2].volume      = 12000;

  // Kick drum
  voices[3].waveforms   = NOISE;
  voices[3].attack_ms   = 5;
  voices[3].decay_ms    = 10;
  voices[3].sustain     = 16000;
  voices[3].release_ms  = 100;
  voices[3].volume      = 18000; // NOISE waveform is very loud
                                 // when using PWM, try lowering
                                 // the volume if it's too noisy
  // Hi-hat
  voices[4].waveforms   = NOISE;
  voices[4].attack_ms   = 5;
  voices[4].decay_ms    = 100;
  voices[4].sustain     = 50;
  voices[4].release_ms  = 40;
  voices[4].volume      = 10000;

  // Change the playback speed:
  // sequencer_set_tempo(128); // Default is 120bpm

  // Change the overall volume (I2S output only):
  set_volume(50); // 0-100, default 100

  // Finally, start the sequencer.
  // Pass true to loop the sequence, false to play it only once.
  sequencer_start(true);

  // You can stop the sequencer with:
  // sequencer_stop();

  // You can also set a callback to execute when the sequence
  // is complete:
  // sequencer_set_callback(your_callback_function);

  while (true) {
    // Nothing to do here, as all processing and
    // audio generation is asyncronous
    tight_loop_contents();
  }

  return 0;
}
