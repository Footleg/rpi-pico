/*
 * A graphics rendering boilerplate project for the Pimoroni Presto.
 * Draws text and circles on the screen using a double buffer.
 * Supporting the full screen resolutions of 480 x 480 with a double buffer by
 * using PSRAM for the drawing buffer. This is significantly slower than using
 * the rp2350 chip RAM but allows full screen resolution graphics to be drawn.
 *
 * NOTE: I found sometimes on uploading a binary to the Presto which uses psram
 * I would just get a blank screen. Resetting or unplugging power and restoring
 * it would clear this state and the graphics in this example would then draw
 * successfully.
 *
 * This boilerplate project also provides touchscreen support with short
 * and long press events, supporting both press-release and press-hold
 * detection.
 *
 * Copyright (c) 2025 Dr Footleg
 *
 * License: GNU GPL v3.0
 */
#include <math.h>
#include <string.h>

#include <cstdlib>

#include "../drivers/touchscreen/touchscreen.hpp"
#include "../libraries/graphics/footleg_graphics.hpp"
#include "drivers/st7701/st7701.hpp"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "libraries/pico_graphics/pico_graphics.hpp"
#include "libraries/pico_vector/pico_vector.hpp"
#include "pico/multicore.h"
#include "pico/platform.h"
#include "pico/sync.h"
#include "pico/time.h"
extern "C" {
#include "sfe_pico_alloc.h"
#include "sfe_psram.h"
}

using namespace pimoroni;

// To get the best graphics quality without exceeding the RAM, use psram for the
// drawing buffer. This is slower to copy to the main chip ram used for the
// display buffer.
#define FRAME_BUFFER_WIDTH 480
#define FRAME_BUFFER_HEIGHT 480

#define DRAW_BUF_SIZE \
  (FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT * sizeof(uint16_t) / 1)

bool DRAW_AA = true;  // Sets whether to antialias the edges of the circles

// This is the resolution of the simulation space (independent of the resolution
// of the screen we decide to render it onto)
static const uint16_t screen_width = 480;
static const uint16_t screen_height = 480;

static const uint BACKLIGHT = 45;
static const uint LCD_CLK = 26;
static const uint LCD_CS = 28;
static const uint LCD_DAT = 27;
static const uint LCD_DC = -1;
static const uint LCD_D0 = 1;

// uint16_t draw_buffer[FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT];
uint16_t* draw_buffer;
uint16_t screen_buffer[FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT];

ST7701* presto;
PicoGraphics_PenRGB565* display;
FootlegGraphics* footlegGraphics;

// Maximum time in ms for a touch and release to be acted on as a 'short press'
static const uint TOUCH_SHORT_PRESS_TIME = 200;
// Time in ms after which a touch is acted on as a 'held action'
static const uint TOUCH_HELD_TIME = 1000;
// Times between the above values will trigger 'long press' actions

// Height and Width in pixels of the areas treated as corner presses on the
// touch screen
static const uint TOUCH_CORNER_SIZE = 60;

uint32_t time() {
  absolute_time_t t = get_absolute_time();
  return to_ms_since_boot(t);
}

int main() {
  char msg[96];  // Make sure buffer for text messages doesn't overflow
  uint16_t frame_counter, lastFC = 0;
  uint64_t start_fps, elapsed, lastSettingsChange;  // Microsecond times
  double fps = 0.0f, prevFps = 0.0f;  // Frames per second to display

  // Initialise random numbers seed using floating adc input reading
  adc_init();
  // Make sure GPIO is high-impedance, no pullups etc
  adc_gpio_init(47);
  // Select ADC input 7 (GPIO47) which is available on the Presto
  adc_select_input(7);
  float seed = 0.8;  // 0.0f;
  for (int it = 0; it < 100; it++) {
    seed += adc_read();
  }
  srand(int(seed));

  // Set up hardware
  gpio_init(LCD_CS);
  gpio_put(LCD_CS, 1);
  gpio_set_dir(LCD_CS, 1);

  sfe_setup_psram(47);
  sfe_pico_alloc_init();

  draw_buffer = (uint16_t*)malloc(DRAW_BUF_SIZE);

  presto = new ST7701(
      FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, ROTATE_0,
      SPIPins{spi1, LCD_CS, LCD_CLK, LCD_DAT, PIN_UNUSED, LCD_DC, BACKLIGHT},
      screen_buffer);
  display = new PicoGraphics_PenRGB565(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT,
                                       draw_buffer);

  footlegGraphics = new FootlegGraphics(display, draw_buffer);

  presto->init();

  static I2C i2c(30, 31, 100000);
  static TouchScreen touch(&i2c);

  Point text_location(5, 5);
  Point lastTouch(0, 0);

  Pen BG = display->create_pen(0, 0, 0);
  Pen WHITE = display->create_pen(255, 255, 255);
  Pen GREY = display->create_pen(96, 96, 96);
  Pen GREEN = display->create_pen(0, 255, 0);
  Pen RED = display->create_pen(255, 0, 0);
  Pen BLUE = display->create_pen(0, 0, 255);

  // Get the start time (used to calculate fps)
  start_fps = time_us_64();
  lastSettingsChange = start_fps;

  bool showText = true;
  bool showGrid = true;

  // Main simulation loop
  while (true) {
    // Check whether the touch screen is being touched right now
    if (touch.read()) {
      // Check how long this touch has been going on for
      uint32_t btn_held_for = touch.held_for();
      if (btn_held_for > TOUCH_HELD_TIME) {
        // Actions to take when touch has been held for over x seconds
        Point touchPoint = touch.last_touched_point();

        if (touchPoint.x > touch.bounds.w - TOUCH_CORNER_SIZE) {
          // Touch on right edge of screen
          lastSettingsChange = time_us_64();
        } else {
          // Create circle at position of touch (until released)
        }
      }
    }

    // Check and act on button press-release events
    {  // Scope block so checkBtn var is cleared on each loop iteration
      uint32_t checkBtn =
          touch.was_released();  // Calling this clears the state, so capture
                                 // the time in a var as we can't ask twice
                                 // for one press
      if (checkBtn > 0) {
        // Button was pressed and released. Take action based on how long it was
        // down for here
        Point touchPoint = touch.last_touched_point();
        if (checkBtn < TOUCH_HELD_TIME) {
          // Touch screen press+release event. Take actions based on where on
          // the screen was clicked (touch screen returns full resolution
          // coordinates so 480 x 480)
          bool actionTaken = false;
          if (touchPoint.x < TOUCH_CORNER_SIZE) {
            // Left side of screen
            if (touchPoint.y < TOUCH_CORNER_SIZE) {
              // Top Left Corner, toggle text visibility
              showText = !showText;
              actionTaken = true;
            } else if (touchPoint.y > touch.bounds.h - TOUCH_CORNER_SIZE) {
              // Bottom Left Corner
              DRAW_AA = !DRAW_AA;
              actionTaken = true;
              lastSettingsChange = time_us_64();
            }
          } else if (touchPoint.x > touch.bounds.w - TOUCH_CORNER_SIZE) {
            // Right side of screen
            if (touchPoint.y < TOUCH_CORNER_SIZE) {
              // Top Right Corner
            } else if (touchPoint.y > touch.bounds.h - TOUCH_CORNER_SIZE) {
              // Bottom Right Corner
              showGrid = !showGrid;
              actionTaken = true;
              lastSettingsChange = time_us_64();
            }
          }

          if (actionTaken == false) {
            // Press/release was not in any special screen area.
            if (checkBtn < TOUCH_SHORT_PRESS_TIME) {
              // Very short touch (under 200ms)
              // Create a ball at position of touch
            } else {
              // Long press anywhere but the screen corners
              // Update the simulation mode.
              lastSettingsChange = time_us_64();
            }
          }
        }
      }
    }

    // Update screen
    display->set_pen(BG);
    display->clear();

    if (showGrid) {
      display->set_pen(GREY);
      for (int x = 8; x < screen_width; x += 8) {
        for (int y = 8; y < screen_height; y += 8) {
          display->set_pixel(Point(x, y));
        }
      }
    }

    // Draw circles at 1:1 scale on screen
    display->set_pen(RED);
    int cx = screen_width;
    int cy = screen_height;
    if (DRAW_AA) {
      footlegGraphics->drawCircleAA(cx / 4, cy / 4, 14, RED);
      footlegGraphics->drawCircleAA(cx / 4, cy / 2, 15, GREEN);
      footlegGraphics->drawCircleAA(cx / 4, cy * 3 / 4, 16, BLUE);
      footlegGraphics->drawCircleAA(cx / 2, cy / 4, 17, RED);
      footlegGraphics->drawCircleAA(cx / 2, cy / 2, 18, GREEN);
      footlegGraphics->drawCircleAA(cx / 2, cy * 3 / 4, 19, BLUE);
      footlegGraphics->drawCircleAA(cx * 3 / 4, cy / 4, 20, RED);
      footlegGraphics->drawCircleAA(cx * 3 / 4, cy / 2, 21, GREEN);
      footlegGraphics->drawCircleAA(cx * 3 / 4, cy * 3 / 4, 13, BLUE);
    } else {
      footlegGraphics->drawCircle(cx / 4, cy / 4, 14, RED);
      footlegGraphics->drawCircle(cx / 4, cy / 2, 15, GREEN);
      footlegGraphics->drawCircle(cx / 4, cy * 3 / 4, 16, BLUE);
      footlegGraphics->drawCircle(cx / 2, cy / 4, 17, RED);
      footlegGraphics->drawCircle(cx / 2, cy / 2, 18, GREEN);
      footlegGraphics->drawCircle(cx / 2, cy * 3 / 4, 19, BLUE);
      footlegGraphics->drawCircle(cx * 3 / 4, cy / 4, 20, RED);
      footlegGraphics->drawCircle(cx * 3 / 4, cy / 2, 21, GREEN);
      footlegGraphics->drawCircle(cx * 3 / 4, cy * 3 / 4, 13, BLUE);
    }

    // Calculate fps
    frame_counter++;
    elapsed = time_us_64();  // Get the current time to calculate fps and
                             // check if reset calc window is needed

    fps = float(frame_counter) * 1000000.0f / float(elapsed - start_fps);

    // Reset times over which fps is calculated every 4 seconds
    if (elapsed - start_fps > 4000000) {
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
    char prefix[48];
    char suffix[48];

    lastTouch = touch.last_touched_point();
    if (DRAW_AA) {
      sprintf(prefix, "WxH:%ix%i AA Touch:%i,%i", FRAME_BUFFER_WIDTH,
              FRAME_BUFFER_HEIGHT, lastTouch.x, lastTouch.y);
    } else {
      sprintf(prefix, "WxH:%ix%i Touch:%i,%i", FRAME_BUFFER_WIDTH,
              FRAME_BUFFER_HEIGHT, lastTouch.x, lastTouch.y);
    }

    sprintf(suffix, " fps:%5.2f", fps);

    // Copy the contents of the first array into the combined array
    strcpy(msg, prefix);
    // Concatenate the contents of the second array into the combined
    // array
    strcat(msg, suffix);

    // Render Mode info and FPS to screen
    display->set_pen(WHITE);

    if (showText || elapsed - lastSettingsChange < 2000000) {
      display->text(msg, text_location, display->bounds.w - text_location.x, 2);
    } else {
      display->set_pixel(Point(4, 4));
    }

    presto->update(display);
  }

  return 0;
}
