/**
 * @file sequencer.c
 * @brief Implementation of the sequencer module.
 */

#include "pico/stdlib.h"
#include "sequencer.h"
#include "synth.h"
#include "pitches.h"
#if USE_AUDIO_PWM
  #include "sound_pwm.h"
#elif USE_AUDIO_I2S
  #include "sound_i2s.h"
#endif

/**
 * @brief The sequencer object.
 */
Sequencer sequencer;

/**
 * @brief The repeating timer object for the sequencer.
 */
repeating_timer_t sequencer_timer;

/**
 * @brief The callback function for the sequencer.
 */
static void noop(void *user_data) { ; }

/**
 * @brief The notes to be played by the sequencer.
 */
int16_t * _notes;

/**
 * @brief The number of voices in the sequencer.
 */
uint8_t num_voices;

/**
 * @brief The beat duration in milliseconds.
 */
uint16_t beat_ms = 125; // 125ms beat = 120bpm

/**
 * @brief Initializes the sequencer module.
 *
 * @param _num_voices The number of voices to initialize.
 * @param notes The notes to be played by the sequencer.
 * @param length The length of the track in beats.
 */
void sequencer_init(uint8_t _num_voices, const int16_t *notes, uint16_t length) {
  sequencer.track_length = length;
  sequencer.beat_ms = beat_ms;
  sequencer.callback = noop;
  sequencer_set_tempo(120);
  num_voices = _num_voices;
  _notes = (int16_t *)notes;
}

/**
 * @brief Starts the sequencer.
 *
 * @param loop Flag indicating whether the sequencer should loop.
 */
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

/**
 * @brief Stops the sequencer.
 */
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

/**
 * @brief Executes the sequencer task.
 */
void sequencer_task(){
  if(!sequencer.playing) { return; }
  static uint16_t prev_beat = 1;
  static uint16_t beat = 0;

  uint64_t tick_ms = (time_us_64() - sequencer.start_time) / 1000;

  beat = (tick_ms / sequencer.beat_ms);
  if(beat >= sequencer.track_length) { // We reached the end of the track
    if(sequencer.loop) { beat = beat % sequencer.track_length; }
    else {
      sequencer_stop();
      sequencer.callback(&sequencer);
      return;
    }
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

/**
 * @brief Timer callback function for the sequencer.
 *
 * @param timer The timer object.
 *
 * @return True if the timer should continue, false otherwise.
 */
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

/**
 * @brief Sets the tempo of the sequencer.
 *
 * @param bpm The tempo in beats per minute.
 */
void sequencer_set_tempo(uint16_t bpm) {
  beat_ms = 60 * 1000 / bpm / 4;
  sequencer.beat_ms = beat_ms;
}

/**
 * @brief Sets the callback function to be executed when the sequencer finishes playing.
 *
 * @param callback The callback function.
 */
void sequencer_set_callback(void (*callback)(void *user_data)) {
    sequencer.callback = callback;
};

