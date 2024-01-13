#include "pico/stdlib.h"
#include "sequencer.h"
#include "synth.h"
#include "pitches.h"
#if USE_AUDIO_PWM
  #include "sound_pwm.h"
#elif USE_AUDIO_I2S
  #include "sound_i2s.h"
#endif

Sequencer sequencer;
repeating_timer_t sequencer_timer;

int16_t * _notes;
uint8_t num_voices;
uint16_t beat_ms = 125; // 125ms beat = 120bpm

void sequencer_init(AudioChannel *channel, uint8_t _num_voices, const int16_t *notes, uint16_t length) {
  sequencer.track_length = length;
  sequencer.beat_ms = beat_ms;
  set_tempo(120);
  num_voices = _num_voices;
  _notes = (int16_t *)notes;
}

void sequencer_start(bool loop) {
  sequencer.start_time = time_us_64();
  sequencer.loop = loop;
  sequencer.playing = true;
  #if USE_AUDIO_PWM
    sound_pwm_start();
  #elif USE_AUDIO_I2S
    sound_i2s_playback_start();
  #endif
  add_repeating_timer_ms(10, seq_timer_callback, NULL, &sequencer_timer);
}

void sequencer_stop() {
  sequencer.playing = false;
#if USE_AUDIO_PWM
  sound_pwm_stop();
#elif USE_AUDIO_I2S
  // Clear i2s buffer
  int16_t *buf_0 = sound_i2s_get_buffer(0);
  int16_t *buf_1 = sound_i2s_get_buffer(1);
  for (int i = 0; i < 2 * SOUND_I2S_BUFFER_NUM_SAMPLES; i++) {
    buf_0[i] = 0;
    buf_1[i] = 0;
  }
#endif
  cancel_repeating_timer(&sequencer_timer);
}

extern AudioChannel channels[CHANNEL_COUNT];

void sequencer_task(){
  if(!sequencer.playing) { return; }
  static uint16_t prev_beat = 1;
  static uint16_t beat = 0;

  uint64_t tick_ms = (time_us_64() - sequencer.start_time) / 1000;

  beat = (tick_ms / sequencer.beat_ms);
  if(beat >= sequencer.track_length) { // We reached the end of the track
    if(sequencer.loop) { beat = beat % sequencer.track_length; }
    else { sequencer_stop(); return; }
  }

  if (beat == prev_beat) return;
  prev_beat = beat;

  for(uint8_t i = 0; i < num_voices; i++) {
    int16_t note = _notes[i*sequencer.track_length + beat];
    if(note > 0) {
      channels[i].frequency = note;
      trigger_attack(&channels[i]);
    } else if (note == -1) {
      trigger_release(&channels[i]);
    }
  }
}

  bool seq_timer_callback(repeating_timer_t *timer) {
#if USE_AUDIO_I2S
    static int16_t *last_buffer;
    int16_t *buffer = sound_i2s_get_next_buffer();
    if (buffer == NULL || buffer == last_buffer) { return true; }
    last_buffer = buffer;
#endif

    sequencer_task();
    
#if USE_AUDIO_I2S
    for (int i = 0; i < SOUND_I2S_BUFFER_NUM_SAMPLES; i++) {
      int16_t level = get_audio_frame();

      // Copy to I2S buffer
      *buffer++ = level;
      *buffer++ = level;
    }
#endif
    return true;
  }

void set_tempo(uint16_t bpm) {
  beat_ms = 60 * 1000 / bpm / 4;
  sequencer.beat_ms = beat_ms;
}