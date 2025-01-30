/*
 * Demo project for button handling for short and long press, plus press and
 * hold interactions Also includes fps calculation which averages over 5 second
 * intervals. Shows the time buttons have been held down, and acts on short
 * button presses to set LED colour while press and hold actions move the text
 * without triggering the LED colour change action of the short presses.
 *
 * Uses an extended version of the button class, derived from the Pimoroni
 * button with added support for the short/long press detection.
 *
 * Copyright (c) 2025 Dr Footleg
 *
 * License: MIT
 */
#include <math.h>
#include <string.h>

#include <cstdlib>
#include <vector>

#include "drivers/st7789/st7789.hpp"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "libraries/pico_display_28/pico_display_28.hpp"
#include "libraries/pico_graphics/pico_graphics.hpp"
#include "mybutton.hpp"
#include "pico/platform.h"
#include "pico/time.h"
#include "rgbled.hpp"

using namespace pimoroni;

ST7789 st7789(320, 240, ROTATE_0, false, get_spi_pins(BG_SPI_FRONT));
PicoGraphics_PenRGB332 graphics(st7789.width, st7789.height, nullptr);

RGBLED led(PicoDisplay28::LED_R, PicoDisplay28::LED_G, PicoDisplay28::LED_B);

MyButton button_a(PicoDisplay28::A);
MyButton button_b(PicoDisplay28::B);
MyButton button_x(PicoDisplay28::X);
MyButton button_y(PicoDisplay28::Y);

uint32_t time() {
  absolute_time_t t = get_absolute_time();
  return to_ms_since_boot(t);
}

int main() {
  char msg[512];          // Make sure buffer don't overflow
  int frame_counter = 0;  // Counter for number of frames displayed
  uint64_t start_fps, start_delay_change, elapsed;  // Microsecond times
  float fps = 0, prevFps = 0;  // Frames per second to display

  // Initialise random numbers seed using floating adc input reading
  adc_init();
  // Make sure GPIO is high-impedance, no pullups etc
  adc_gpio_init(28);
  // Select ADC input 2 (GPIO28) which is available on the Tufty2040 board
  adc_select_input(2);
  float seed = 0.0f;
  for (int it = 0; it < 100; it++) {
    seed += adc_read();
  }
  srand(int(seed));

  st7789.set_backlight(255);
  led.set_rgb(0, 0, 0);

  Point text_location(0, 0);

  Pen BG = graphics.create_pen(0, 120, 0);
  Pen WHITE = graphics.create_pen(255, 255, 255);

  // Get the start time (used to calculate fps)
  start_fps = time_us_64();

  // Variables used in application (not needed for button management, just for
  // generating information to debug)
  uint16_t delay_ms = 1;
  float btnHeldDownTime_A = 0.0;
  float btnHeldDownTime_B = 0.0;
  float btnHeldDownTime_X = 0.0;
  float btnHeldDownTime_Y = 0.0;

  while (true) {
    // Check the states of the buttons. These blocks check the button states and
    // time how long they were pressed for. When a button is detected to be held
    // down, the time it has been down is read, and actions can be taken based
    // on the duration of the button hold. Actions taken when the button is
    // released are handled afterwards in code further down, where actions can
    // be taken based on hold long the button was pressed for before being
    // released.

    // Check button states - Button A
    if (button_a.read()) {
      // Check how long button was pressed to determine whether to take an
      // action
      uint32_t btn_held_for = button_a.held_for();
      if (btn_held_for > 500) {
        // Actions to take when button has been held down for over 0.5 seconds
        // Magnitude incremented over longer time intervals
        uint8_t txtMoveAmount = 1;
        if (btn_held_for > 4000) {
          txtMoveAmount = 8;
        } else if (btn_held_for > 2000) {
          txtMoveAmount = 4;
        }
        text_location.x += txtMoveAmount;
        if (text_location.x > graphics.bounds.w)
          text_location.x = graphics.bounds.w;
      }
    }

    // Check button states - Button B
    if (button_b.read()) {
      // Check how long button was pressed to determine whether to take an
      // action
      uint32_t btn_held_for = button_b.held_for();
      if (btn_held_for > 500) {
        // Actions to take when button has been held down for over 0.5 seconds
        // Magnitude incremented over longer time intervals
        uint8_t txtMoveAmount = 1;
        if (btn_held_for > 4000) {
          txtMoveAmount = 8;
        } else if (btn_held_for > 2000) {
          txtMoveAmount = 4;
        }
        text_location.x -= txtMoveAmount;
        if (text_location.x < 0) text_location.x = 0;
      }
    }

    // Check button states - Button X
    if (button_x.read()) {
      // Check how long button was pressed to determine whether to take an
      // action
      uint32_t btn_held_for = button_x.held_for();
      if (btn_held_for > 500) {
        // Actions to take when button has been held down for over 0.5 seconds
        // Magnitude incremented over longer time intervals
        uint8_t txtMoveAmount = 1;
        if (btn_held_for > 4000) {
          txtMoveAmount = 8;
        } else if (btn_held_for > 2000) {
          txtMoveAmount = 4;
        }
        text_location.y -= txtMoveAmount;
        if (text_location.y < 0) text_location.y = 0;
      }
    }

    // Check button states - Button Y
    if (button_y.read()) {
      // Check how long button was pressed to determine whether to take an
      // action
      uint32_t btn_held_for = button_y.held_for();
      if (btn_held_for > 500) {
        // Actions to take when button has been held down for over 0.5 seconds
        // Magnitude incremented over longer time intervals
        uint8_t txtMoveAmount = 1;
        if (btn_held_for > 4000) {
          txtMoveAmount = 8;
        } else if (btn_held_for > 2000) {
          txtMoveAmount = 4;
        }
        text_location.y += txtMoveAmount;
        if (text_location.y > graphics.bounds.h)
          text_location.y = graphics.bounds.h;
      }
    }

    // Do any processing here that needs doing before acting on button 'press
    // and release' events

    // Check and act on button press-release events
    {  // Scope block so checkBtn var is cleared on each loop iteration
      uint32_t checkBtn =
          button_a.was_released();  // Calling this clears the state, so capture
                                    // the time in a var as we can't ask twice
                                    // for one press
      if (checkBtn > 0) {
        // Button was pressed and released. Take action based on how long it was
        // down for here
        btnHeldDownTime_A = (float)checkBtn / 1000;
        if (checkBtn < 250) {
          // Short button press (under 0.25 seconds).
          led.set_rgb(128, 128, 0);  // Set RGB LED colour
        }
      }

      checkBtn = button_b.was_released();  // Calling this clears the state, so
                                           // capture the time in a var as we
                                           // can't ask twice for one press
      if (checkBtn > 0) {
        // Button was pressed and released. Take action based on how long it was
        // down for here
        btnHeldDownTime_B = (float)checkBtn / 1000;
        if (checkBtn < 250) {
          // Short button press (under 0.25 seconds).
          led.set_rgb(0, 128, 0);  // Set RGB LED colour
        }
      }

      checkBtn = button_x.was_released();  // Calling this clears the state, so
                                           // capture the time in a var as we
                                           // can't ask twice for one press
      if (checkBtn > 0) {
        // Button was pressed and released. Take action based on how long it was
        // down for here
        btnHeldDownTime_X = (float)checkBtn / 1000;
        if (checkBtn < 250) {
          // Short button press (under 0.25 seconds).
          led.set_rgb(0, 0, 128);  // Set RGB LED colour
        }
      }

      checkBtn = button_y.was_released();  // Calling this clears the state, so
                                           // capture the time in a var as we
                                           // can't ask twice for one press
      if (checkBtn > 0) {
        // Button was pressed and released. Take action based on how long it was
        // down for here
        btnHeldDownTime_Y = (float)checkBtn / 1000;
        if (checkBtn < 250) {
          // Short button press (under 0.25 seconds).
          led.set_rgb(128, 0, 128);  // Set RGB LED colour
        }
      }
    }

    // Clear display ready to redraw
    graphics.set_pen(BG);
    graphics.clear();

    // Update text for information shown on screen
    sprintf(msg, "Frames:%i, Delay:%i, FPS:%5.2f", frame_counter, delay_ms,
            fps);

    // Render Mode info and FPS to screen
    graphics.set_pen(WHITE);
    graphics.text(msg, text_location, 320, 2);

    sprintf(msg,
            "Btns held:\nA:%5.2f, %ims\nB:%5.2f, %ims\nX:%5.2f, %ims\nY:%5.2f, "
            "%ims",
            btnHeldDownTime_A, button_a.held_for(), btnHeldDownTime_B,
            button_b.held_for(), btnHeldDownTime_X, button_x.held_for(),
            btnHeldDownTime_Y, button_y.held_for());
    Point line2(text_location.x, text_location.y + 20);
    graphics.text(msg, line2, 320, 2);

    // update screen
    st7789.update(&graphics);

    // Calculate fps
    frame_counter++;
    elapsed = time_us_64();  // Get the current time to calculate fps and check
                             // if reset calc window is needed

    fps = (frame_counter / float(elapsed - start_fps)) * 1000000;

    // Reset times over which fps is calculated every 1 seconds
    if (elapsed - start_fps > 1000000) {
      frame_counter = 0;
      start_fps = elapsed;
      prevFps = fps;
    } else {
      // Average with 10% weighting of previous calc fps if we have one
      if (prevFps > 0.0) {
        fps = (fps + prevFps * 0.1) / 1.1;
      }
    }

    if (elapsed - start_delay_change > 5000000) {
      // Set a random delay to change the fps every 5 seconds so we can see it
      // update
      delay_ms = rand() % 50;
      start_delay_change = elapsed;
    }

    sleep_ms(delay_ms);
  }

  return 0;
}
