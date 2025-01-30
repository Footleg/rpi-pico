/*
 * Exetended version of the Pimoroni Button class, adding support for reading
 * the duration of long presses and time the button has been held down. Allows
 * short and long presses to be distinguished to trigger different results, and
 * to change program behaviour on holding down buttons based on how long the
 * button has been held.
 *
 * Copyright (c) 2025 Dr Footleg
 *
 * License: MIT
 */
#pragma once

#include <stdint.h>

#include "common/pimoroni_common.hpp"
#include "pico/stdlib.h"

namespace pimoroni {

class MyButton {
 public:
  MyButton(uint pin, Polarity polarity = Polarity::ACTIVE_LOW,
           uint32_t repeat_time = 200, uint32_t hold_time = 1000)
      : pin(pin),
        polarity(polarity),
        repeat_time(repeat_time),
        hold_time(hold_time) {
    gpio_set_function(pin, GPIO_FUNC_SIO);
    gpio_set_dir(pin, GPIO_IN);
    if (polarity == Polarity::ACTIVE_LOW) {
      gpio_pull_up(pin);
    } else {
      gpio_pull_down(pin);
    }
  };
  bool raw();
  bool read();
  uint32_t held_for();
  uint32_t was_released();

 protected:
  bool pressed = false;
  uint32_t pressed_time = 0;

 private:
  uint pin;
  Polarity polarity;
  uint32_t repeat_time;
  uint32_t hold_time;
  bool last_state = false;
  uint32_t last_time = 0;
  uint32_t lastPressedFor = 0;
};

}  // namespace pimoroni
