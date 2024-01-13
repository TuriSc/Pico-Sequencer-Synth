#include "pico/stdlib.h"
#include "sound_pwm.h"
#include "synth.h"
#include "sequencer.h"

#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"

static uint8_t slice_num;
static uint8_t audio_gpio;
static uint8_t pwm_channel;

void sound_pwm_init(uint16_t audio_pin, uint32_t sample_rate) {
  audio_gpio = audio_pin;

  gpio_set_function(audio_pin, GPIO_FUNC_PWM);
  slice_num = pwm_gpio_to_slice_num(audio_pin);

  pwm_clear_irq(slice_num);
  pwm_set_irq_enabled(slice_num, true);
  irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_isr);
  irq_set_enabled(PWM_IRQ_WRAP, true);

  static const uint16_t wrap = 2048;
  pwm_set_clkdiv(slice_num, clock_get_hz(clk_sys) / (float)(wrap * sample_rate));
  pwm_set_wrap(slice_num, wrap);

  pwm_channel = pwm_gpio_to_channel(audio_pin);
  pwm_set_chan_level(slice_num, pwm_channel, 0);
  pwm_set_enabled(slice_num, true);
}

void sound_pwm_start() {
  pwm_set_enabled(slice_num, true);
}

void sound_pwm_stop() {
  pwm_set_chan_level(slice_num, pwm_channel, 0);
  pwm_set_enabled(slice_num, false);
}

void pwm_isr() {
  pwm_clear_irq(slice_num);
  uint16_t level = get_audio_frame();
  pwm_set_chan_level(slice_num, pwm_channel, level);  
}