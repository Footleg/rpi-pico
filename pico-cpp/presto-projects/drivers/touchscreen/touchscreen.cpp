/*
 * Touchscreen wrapper class with support for reading the duration of presses 
 * and time the touchpoint has been held down. Allows short and long presses 
 * to be distinguished to enable program behaviour on screen press-release 
 * and press-hold based on duration of touch.
 *
 * Copyright (c) 2025 Dr Footleg
 *
 * License: MIT
 */

#include "touchscreen.hpp"

bool TouchScreen::read() {
  auto time = millis();
  touch.ft6x36_read(&touchData);
  bool changed = touchData.state != last_state;
  last_state = touchData.state;

  if (changed) {
    if (touchData.state == TOUCH_STATE_PRESSED) {
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

uint32_t TouchScreen::held_for() {
  // Returns the amount of time the button has been held down for while button
  // is being held down
  if (pressed) {
    return millis() - pressed_time;
  } else {
    return 0;
  }
}

uint32_t TouchScreen::was_released() {
  // Returns how touch was held for, once it is released.
  // Calling this method resets the state to zero, so it is an ask once per
  // press affair.
  uint32_t result = lastPressedFor;

  // Clear the stored pressed for time, so we only action the press event once
  // in the calling application
  lastPressedFor = 0;

  return result;
}

Point TouchScreen::last_touched_point(){
  return touchData.point;
}
