#ifndef SYNTH_SEQUENCER_H
#define SYNTH_SEQUENCER_H

/**
 * @file synth_sequencer.h
 * @brief Header file for the synth sequencer module.
 */

#include "synth.h"
#include "sequencer.h"
#include "pitches.h"

#if defined USE_AUDIO_PWM && defined USE_AUDIO_I2S
  #error "You need to define exactly one audio output"
#endif
#ifndef USE_AUDIO_PWM
  #ifndef USE_AUDIO_I2S
    #error "You need to define exactly one audio output"
  #endif
#endif

#if USE_AUDIO_PWM
  #include "sound_pwm.h"
#elif USE_AUDIO_I2S
  #include "sound_i2s.h"
#endif

#endif

