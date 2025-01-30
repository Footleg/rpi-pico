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

#include "mybutton.hpp"

namespace pimoroni {
bool MyButton::raw() {
  if (polarity == Polarity::ACTIVE_LOW) {
    return !gpio_get(pin);
  } else {
    return gpio_get(pin);
  }
}

bool MyButton::read() {
  auto time = millis();
  bool state = raw();
  bool changed = state != last_state;
  last_state = state;

  if (changed) {
    if (state) {
      pressed_time = time;
      pressed = true;
      last_time = time;
      return true;
    } else {
      // Button has been released after a press, store how long it was held down
      lastPressedFor = millis() - pressed_time;
      pressed_time = 0;
      pressed = false;
      last_time = 0;
    }
  }
  // Shortcut for no auto-repeat
  if (repeat_time == 0) return false;

  if (pressed) {
    uint32_t repeat_rate = repeat_time;
    if (hold_time > 0 && time - pressed_time > hold_time) {
      repeat_rate /= 3;
    }
    if (time - last_time > repeat_rate) {
      last_time = time;
      return true;
    }
  }

  return false;
}

uint32_t MyButton::held_for() {
  // Returns the amount of time the button has been held down for while button
  // is being held down
  if (pressed) {
    return millis() - pressed_time;
  } else {
    return 0;
  }
}

uint32_t MyButton::was_released() {
  // Returns how long a button was pressed for, once it is released.
  // Calling this method resets the state to zero, so it is an ask once per
  // press affair.
  uint32_t result = lastPressedFor;

  // Clear the stored pressed for time, so we only action the press event once
  // in the calling application
  lastPressedFor = 0;

  return result;
}

}  // namespace pimoroni
