#ifndef SEQUENCER_H
#define SEQUENCER_H

/**
 * @file sequencer.h
 * @brief Header file for the sequencer module.
 */

#include "pico/stdlib.h"
#include "synth.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct Sequencer
 * @brief Represents a sequencer object.
 */
typedef struct Sequencer {
  /**
   * @brief Callback function to be executed when the sequencer finishes playing.
   *
   * @param user_data User-defined data to be passed to the callback function.
   */
  void (*callback)(void *user_data);

  /**
   * @brief The length of the track in beats.
   */
  uint16_t  track_length;

  /**
   * @brief The start time of the sequencer in microseconds.
   */
  uint64_t start_time;

  /**
   * @brief The beat duration in milliseconds.
   */
  uint16_t beat_ms;

  /**
   * @brief Flag indicating whether the sequencer is playing.
   */
  bool playing;

  /**
   * @brief Flag indicating whether the sequencer should loop.
   */
  bool loop;
} Sequencer;

/**
 * @brief Initializes the sequencer module.
 *
 * @param _num_voices The number of voices to initialize.
 * @param notes The notes to be played by the sequencer.
 * @param length The length of the track in beats.
 */
void sequencer_init(uint8_t _num_voices, const int16_t *notes, uint16_t length);

/**
 * @brief Starts the sequencer.
 *
 * @param loop Flag indicating whether the sequencer should loop.
 */
void sequencer_start(bool loop);

/**
 * @brief Stops the sequencer.
 */
void sequencer_stop();

/**
 * @brief Executes the sequencer task.
 */
void sequencer_task();

/**
 * @brief Sets the tempo of the sequencer.
 *
 * @param bpm The tempo in beats per minute.
 */
void sequencer_set_tempo(uint16_t bpm);

/**
 * @brief Sets the callback function to be executed when the sequencer finishes playing.
 *
 * @param callback The callback function.
 */
void sequencer_set_callback(void (*callback)(void *user_data));

/**
 * @brief Timer callback function for the sequencer.
 *
 * @param timer The timer object.
 *
 * @return True if the timer should continue, false otherwise.
 */
bool seq_timer_callback(repeating_timer_t *timer);

#ifdef __cplusplus
}
#endif

#endif

