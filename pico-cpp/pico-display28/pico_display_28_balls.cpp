/*
 * A bouncing balls simulation which started by me adding collisions to the
 * Pimoroni balls demo, but then I added attractive/repulsive forces as an
 * option and it became a fun simulation of spheres interacting. Ported from my
 * tufty2040 example, but extended with improved controls using the extended
 * button class to support long press and short press events.
 *
 * Copyright (c) 2025 Dr Footleg
 *
 * License: GNU GPL v3.0
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

const uint8_t MODE_BOUNCE = 0;
const uint8_t MODE_FORCES = 1;

uint32_t time() {
  absolute_time_t t = get_absolute_time();
  return to_ms_since_boot(t);
}

struct pt {
  float x;
  float y;
  uint8_t r;
  float dx;
  float dy;
  uint16_t pen;
};

pt createShape() {
  pt shape;
  shape.x = rand() % graphics.bounds.w;
  shape.y = rand() % graphics.bounds.h;
  shape.r = (rand() % 20) + 2;
  shape.dx = float(rand() % 255) / 64.0f;
  shape.dy = float(rand() % 255) / 64.0f;
  // Generate random colour which is not too dark
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  while (r + g + b < 192) {
    r = rand() % 255;
    g = rand() % 255;
    b = rand() % 255;
  }
  shape.pen = graphics.create_pen(r, g, b);
  return shape;
};

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

  Point text_location(5, 5);

  Pen BG = graphics.create_pen(0, 0, 0);
  Pen WHITE = graphics.create_pen(255, 255, 255);

  // Get the start time (used to calculate fps)
  start_fps = time_us_64();

  std::vector<pt> shapes;

  // Create 2 spheres initially
  for (int i = 0; i < 2; i++) {
    pt newShape = createShape();
    shapes.push_back(newShape);
  }

  // Simulation boundaries
  float minX = 0;
  float minY = 0;
  float maxX = graphics.bounds.w;
  float maxY = graphics.bounds.h;
  uint8_t renderSkip = 1;
  uint8_t renderCount = 0;

  uint16_t i = 0;
  uint8_t mode = MODE_BOUNCE;
  float forcePower = 2.0f;
  float step = 2.0f;
  bool mass = true;

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
        text_location.y -= txtMoveAmount;
        if (text_location.y < 0) text_location.y = 0;
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
        text_location.y += txtMoveAmount;
        if (text_location.y > graphics.bounds.h)
          text_location.y = graphics.bounds.h;
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
        float forceIncrement = step;
        if (btn_held_for > 8000) {
          forceIncrement = step * btn_held_for / 1000;
        } else if (btn_held_for > 4000) {
          forceIncrement = step * 4;
        } else if (btn_held_for > 2000) {
          forceIncrement = step * 2;
        }
        forcePower += forceIncrement;
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
        float forceIncrement = step;
        if (btn_held_for > 8000) {
          forceIncrement = step * btn_held_for / 1000;
        } else if (btn_held_for > 4000) {
          forceIncrement = step * 4;
        } else if (btn_held_for > 2000) {
          forceIncrement = step * 2;
        }
        forcePower -= forceIncrement;
      }
    }

    // Do any processing here that needs doing before acting on button 'press
    // and release' events

    // Clear display ready to redraw (only on render loop cycles)
    if (renderCount == 0) {
      graphics.set_pen(BG);
      graphics.clear();
    }

    i = 0;
    for (auto &shape : shapes) {
      // Update shape position
      shape.x += shape.dx;
      shape.y += shape.dy;

      // Check for collision with shapes already updated
      for (uint8_t j = 0; j < i; j++) {
        // Check distance between shapes
        float sepx = shapes[j].x - shape.x;
        float sepy = shapes[j].y - shape.y;
        uint16_t sep = int(sqrt((sepx * sepx) + (sepy * sepy)));

        // Don't try to process interactions if shapes exactly on top of one
        // another
        if (sep > 0.0) {
          float ax = 0.0f;
          float ay = 0.0f;

          uint8_t rd = 0;
          float force = forcePower;

          // Bounce if contacting
          rd = shape.r + shapes[j].r;
          if (sep < rd) {
            // If forces between balls, allow to pass each other when
            // overlapping, unless centres really close. Don't apply forces
            // during overlap as force is too strong and they just stick
            // together.
            if (mode == MODE_BOUNCE || sep < rd / 4) {
              // Bounce balls off each other
              /*
              //Trig solution
              float angle = atan2(sepy, sepx);
              float targetX = shape.x + cos(angle) * sep;
              float targetY = shape.y + sin(angle) * sep;
              float ax = (targetX - shape.x);
              float ay = (targetY - shape.y);
              */

              // Handle x and y components seperately (more efficient)
              ax = sepx;
              ay = sepy;
            }
          } else {
            switch (mode) {
              case MODE_FORCES:
                // Repel, Force is inverse of distance squared
                force = force / (sep * sep);
                ax = force * sepx / sep;
                ay = force * sepy / sep;

                break;
            }
          }

          float prePower =
              sqrt(shape.dx * shape.dx + shape.dy * shape.dy) +
              sqrt(shapes[j].dx * shapes[j].dx + shapes[j].dy * shapes[j].dy);
          if (mass) {
            shape.dx -= ax * shapes[j].r;
            shape.dy -= ay * shapes[j].r;
            shapes[j].dx += ax * shape.r;
            shapes[j].dy += ay * shape.r;
          } else {
            shape.dx -= ax * 10;
            shape.dy -= ay * 10;
            shapes[j].dx += ax * 10;
            shapes[j].dy += ay * 10;
          }

          float postPower =
              sqrt(shape.dx * shape.dx + shape.dy * shape.dy) +
              sqrt(shapes[j].dx * shapes[j].dx + shapes[j].dy * shapes[j].dy);
          float scalePower = prePower / postPower;

          shape.dx = shape.dx * scalePower;
          shape.dy = shape.dy * scalePower;
          shapes[j].dx = shapes[j].dx * scalePower;
          shapes[j].dy = shapes[j].dy * scalePower;
        }
      }

      // Check shape remains in bounds of screen, reverse direction if not
      if ((shape.x - shape.r) < minX) {
        shape.dx *= -1;
        shape.x = minX + shape.r;
      }
      if ((shape.x + shape.r) >= maxX) {
        shape.dx *= -1;
        shape.x = maxX - shape.r;
      }
      if ((shape.y - shape.r) < minY) {
        shape.dy *= -1;
        shape.y = minY + shape.r;
      }
      if ((shape.y + shape.r) >= maxY) {
        shape.dy *= -1;
        shape.y = maxY - shape.r;
      }

      if (renderCount == 0) {
        // Skip the slow calcs if 1:1 scale with screen
        if (minX == 0 && minY == 0) {
          // Draw circles at 1:1 scale on screen
          graphics.set_pen(shape.pen);
          graphics.circle(Point(shape.x, shape.y), shape.r);
        } else {
          // Draw circles scaled to boundaries
          float posX = graphics.bounds.w * (shape.x - minX) / (maxX - minX);
          float posY = graphics.bounds.h * (shape.y - minY) / (maxY - minY);
          float rad = graphics.bounds.h * shape.r / (maxY - minY);
          if (rad < 2) rad = 2;
          graphics.set_pen(shape.pen);
          graphics.circle(Point(posX, posY), rad);
        }
      }

      i++;
    }

    if (renderCount == 0) {
      // Update screen on render loop cycles
      // Calculate fps
      frame_counter++;
      elapsed = time_us_64();  // Get the current time to calculate fps and
                               // check if reset calc window is needed

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

      // Update text for information shown on screen
      if (mass) {
        switch (mode) {
          case MODE_BOUNCE:
            sprintf(msg, "Bounce (M), Balls:%i; FPS = %5.2f", shapes.size(),
                    fps);
            break;
          case MODE_FORCES:
            sprintf(msg, "Force %.1f (M), Balls:%i; FPS = %5.2f", forcePower,
                    shapes.size(), fps);
            break;
          default:
            sprintf(msg, "Unsupported Mode!, FPS = %5.2f", fps);
        }
      } else {
        switch (mode) {
          case MODE_BOUNCE:
            sprintf(msg, "Bounce, Balls:%i; FPS = %5.2f", shapes.size(), fps);
            break;
          case MODE_FORCES:
            sprintf(msg, "Force %.1f, Balls:%i; FPS = %5.2f", forcePower,
                    shapes.size(), fps);
            break;
          default:
            sprintf(msg, "Unsupported Mode!, FPS = %5.2f", fps);
        }
      }

      // Render Mode info and FPS to screen
      graphics.set_pen(WHITE);
      graphics.text(msg, text_location, 320, 2);

      // update screen
      st7789.update(&graphics);
    }

    // Check and act on button press-release events
    {  // Scope block so checkBtn var is cleared on each loop iteration
      uint32_t checkBtn =
          button_a.was_released();  // Calling this clears the state, so capture
                                    // the time in a var as we can't ask twice
                                    // for one press
      if (checkBtn > 0) {
        // Button was pressed and released. Take action based on how long it was
        // down for here
        if (checkBtn < 600) {
          // Short button press (under 0.6 seconds). Update the simulation
          // mode.
          mode++;
          if (mode > 1) mode = 0;
        }
      }

      checkBtn = button_b.was_released();  // Calling this clears the state, so
                                           // capture the time in a var as we
                                           // can't ask twice for one press
      if (checkBtn > 0) {
        // Button was pressed and released. Take action based on how long it was
        // down for here
        if (checkBtn < 600) {
          // Short button press (under 0.6 seconds). Add a new ball to the
          // simulation.
          pt newShape = createShape();
          shapes.push_back(newShape);
        }
      }

      checkBtn = button_x.was_released();  // Calling this clears the state, so
                                           // capture the time in a var as we
                                           // can't ask twice for one press
      if (checkBtn > 0) {
        // Button was pressed and released. Take action based on how long it was
        // down for here
        if (checkBtn < 600) {
          // Short button press (under 0.6 seconds).
          // Find current bounds and multiply by 1.5
          float minXc = shapes[0].x;
          float minYc = shapes[0].y;
          float maxXc = minXc;
          float maxYc = minYc;
          for (auto &shape : shapes) {
            if (shape.x < minXc) minXc = shape.x;
            if (shape.x > maxXc) maxXc = shape.x;
            if (shape.y < minYc) minYc = shape.y;
            if (shape.y > maxYc) maxYc = shape.y;
          }
          float midX = minXc + (maxXc - minXc) / 2;
          float midY = minYc + (maxYc - minYc) / 2;
          float addX = (maxX - minX) * 1.5 / 2;
          float addY = (maxY - minY) * 1.5 / 2;
          minX -= addX;
          maxX += addX;
          minY -= addY;
          maxY += addY;

          renderSkip++;
        }
      }

      checkBtn = button_y.was_released();  // Calling this clears the state, so
                                           // capture the time in a var as we
                                           // can't ask twice for one press
      if (checkBtn > 0) {
        // Button was pressed and released. Take action based on how long it was
        // down for here
        if (checkBtn < 600) {
          // Short button press (under 0.6 seconds).
          // Find current bounds shink area to just include them
          float minXc = shapes[0].x;
          float minYc = shapes[0].y;
          float maxXc = minXc;
          float maxYc = minYc;
          for (auto &shape : shapes) {
            if (shape.x < minXc) minXc = shape.x;
            if (shape.x > maxXc) maxXc = shape.x;
            if (shape.y < minYc) minYc = shape.y;
            if (shape.y > maxYc) maxYc = shape.y;
          }
          float midX = minXc + (maxXc - minXc) / 2;
          float midY = minYc + (maxYc - minYc) / 2;
          float totX = maxXc - minXc;
          float totY = maxYc - minYc;
          if (totX > totY * graphics.bounds.w / graphics.bounds.h) {
            totY = totX * graphics.bounds.h / graphics.bounds.w;
          } else {
            totX = totY * graphics.bounds.w / graphics.bounds.h;
          }
          minX = midX - totX / 1.95;
          maxX = midX + totX / 1.95;
          minY = midY - totY / 1.95;
          maxY = midY + totY / 1.95;

          if (renderSkip > 0) renderSkip--;

          if (maxX - minX < graphics.bounds.w) {
            // Reset to 1:1 scale
            minX = 0;
            minY = 0;
            maxX = graphics.bounds.w;
            maxY = graphics.bounds.h;
            renderSkip = 1;
          }
        }
      }
    }

    if (abs(forcePower) < 4) {
      step = 0.1;
    } else if (abs(forcePower) < 20) {
      step = 1.0;
    } else if (abs(forcePower) < 40) {
      step = 2.0;
    } else if (abs(forcePower) < 80) {
      step = 4.0;
    } else {
      step = 10.0;
    }

    // Increment render counter (graphics are only rendered on loop cycles where
    // this rolls over to zero)
    renderCount++;
    if (renderCount > renderSkip) renderCount = 0;
  }

  return 0;
}
