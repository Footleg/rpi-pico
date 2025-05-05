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
#pragma once

#include <stdint.h>

#include "../ft6x36/ft6x36.h"
#include "libraries/pico_graphics/pico_graphics.hpp"

struct Bounds {
  uint16_t h = 0, w = 0;
};

class TouchScreen {
public:
  TouchScreen(I2C *i2c, uint32_t repeat_time = 200, uint32_t hold_time = 1000)
      : touch(i2c, 0x48), repeat_time(repeat_time), hold_time(hold_time) {};
  bool read();
  uint32_t held_for();
  uint32_t was_released();
  Point last_touched_point();
  const Bounds bounds = {480, 480};

protected:
  bool pressed = false;
  uint32_t pressed_time = 0;

private:
  FT6X36 touch;
  touch_data_t touchData;
  uint32_t repeat_time;
  uint32_t hold_time;
  touch_state_t last_state = TOUCH_STATE_RELEASED;
  uint32_t last_time = 0;
  uint32_t lastPressedFor = 0;
};
