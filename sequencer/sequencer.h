#ifndef SEQUENCER_H
#define SEQUENCER_H
#include "pico/stdlib.h"
#include "synth.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Sequencer {
  uint16_t  track_length;
  uint64_t start_time;
  uint16_t beat_ms;
  bool playing;
  bool loop;
} Sequencer;

void sequencer_init(uint8_t _num_voices, const int16_t *notes, uint16_t length);
void sequencer_start(bool loop);
void sequencer_stop();
void sequencer_task();
void set_tempo(uint16_t bpm);
bool seq_timer_callback(repeating_timer_t *timer);

#ifdef __cplusplus
}
#endif

#endif
