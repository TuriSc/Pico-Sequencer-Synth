#ifndef SOUND_PWM_H
#define SOUND_PWM_H
#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SAMPLES_PER_BUFFER 256

extern void update_playback(void);

void sound_pwm_init(uint16_t audio_pin, uint32_t sample_rate);
void sound_pwm_start();
void sound_pwm_stop();
void pwm_isr();

#ifdef __cplusplus
}
#endif

#endif
