/*
 * A graphics rendering boilerplate project for the Pimoroni Presto.
 * Draws text and circles on the screen using a double buffer.
 * Supporting buffer resolutions of 240 x 240,  240 x 480 and 480 x 240
 * (There is not enough RAM to support a double buffer of 480 x 480).
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
#include "drivers/st7701/st7701.hpp"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "libraries/pico_graphics/pico_graphics.hpp"
#include "libraries/pico_vector/pico_vector.hpp"
#include "pico/multicore.h"
#include "pico/platform.h"
#include "pico/sync.h"
#include "pico/time.h"
#include "../libraries/graphics/footleg_graphics.hpp"
// #include "pimoroni_i2c.hpp"

using namespace pimoroni;

// To get the best graphics quality without exceeding the RAM, use a width of
// 480 with a height of 240. All drawing commands will be done on a 480 x 480 screen
// size, and scaled accordingly to draw round circles on the screen regardless of the 
// buffer aspect ratio used.
#define FRAME_BUFFER_WIDTH 240
#define FRAME_BUFFER_HEIGHT 480

bool DRAW_AA = true;  // Sets whether to antialias the edges of the circles

// To help debug graphics, pixels can be rendered as a multiple of actual screen pixels.
// e.g. A setting of 2 will draw each pixel using 2x2 on screen pixels.
int pixelMultiplier = 2; 

int debugY = 0;

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

uint16_t back_buffer[FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT];
uint16_t front_buffer[FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT];

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

float debug1, debug2, debug3 = 0.0;

uint32_t time() {
  absolute_time_t t = get_absolute_time();
  return to_ms_since_boot(t);
}

void drawPixel(Point p) {
  if (pixelMultiplier == 1) {
    // Normal single pixel
    display->set_pixel(p);
  } else {
    // Multiplied pixel
    int x = p.x * pixelMultiplier - pixelMultiplier + 1;
    int y = p.y * pixelMultiplier - pixelMultiplier + 1;

    for (int xi = 0; xi < pixelMultiplier; xi++) {
      for (int yi = 0; yi < pixelMultiplier; yi++) {
        display->set_pixel(Point(x + xi, y + yi));
      }
    }
  }
}

void drawPixelSpan(Point p, int width) {
  if (pixelMultiplier == 1) {
    // Normal pixel span
    if(width > 0) display->pixel_span(p, width);
  } else {
    // Multiplied pixels span 
    for (int xi = 0; xi < width; xi++) {
      drawPixel(Point(p.x + xi, p.y));
    }
  }
}

void drawAASpan(float x, int y, float width, uint16_t pen) {
  int iX1 = std::floor(x);
  float spanX = 1 - (x - iX1);  // How much line extends over end pixel
  // debug1 = lineX;
  // debug2 = lineX + width;
  // debug3 = spanX;
  if (spanX != 1) {
    // Determine AA pixel at end of span
    int iX2 = std::floor(x + width * 2);
    // Set tick at original start and end
    RGB rgb = PicoGraphics::rgb565_to_rgb(pen);
    // Antialias start and end pixels into black only if background is black
    display->set_pen(
        display->create_pen(rgb.r * spanX, rgb.g * spanX, rgb.b * spanX));
    if (front_buffer[y * FRAME_BUFFER_WIDTH + iX1] >= 0) drawPixel(Point(iX1, y));
    if (front_buffer[y * FRAME_BUFFER_WIDTH + iX2] >= 0) drawPixel(Point(iX2, y));
    // front_buffer[y * FRAME_BUFFER_WIDTH + iX1] = 255*8; // DEBUG force test pixel to
    // show front_buffer[y * FRAME_BUFFER_WIDTH + iX2] = 255*8; // DEBUG force test
    // pixel to show

    // if (width > 1 && width <= 2) {
    //   debug1 = x;
    //   debug2 = y;
    //   debug3 = width;
    // }

    // if (y % 2) {
    //   display->set_pen(display->create_pen(80, 50, 0));
    // } else {
    //   display->set_pen(display->create_pen(0, 20, 80));
    // }
    // Reduce span to not overwrite AA dimmed end pixels
    display->set_pen(pen);
    drawPixelSpan(Point(iX1 + 1, y), iX2 - iX1 - 1);
  } else {
    // Solid line, so include end pixels in span
    display->set_pen(pen);
    // display->set_pen(display->create_pen(120, 50, 0));
    drawPixelSpan(Point(iX1, y), std::round(width * 2));
  }
}

void drawCircleAA(int centreX, int centreY, int rad, uint16_t pen) {
  // Point position =
  //     Point(x * FRAME_BUFFER_WIDTH / screen_width, y * FRAME_BUFFER_HEIGHT /
  //     screen_height);

  // display->circle(position, rad * FRAME_BUFFER_WIDTH / screen_width);

  // centreX = 240;  // DEBUG FIXED IN CENTRE OF SCREEN
  // centreY = 240;  // DEBUG FIXED IN CENTRE OF SCREEN
  // centreX = 80;  // DEBUG FIXED IN CENTRE OF SCREEN with 3x3 pixels
  // centreY = 80;  // DEBUG FIXED IN CENTRE OF SCREEN

  // Convert radius to pixel scale
  float scaledRadX = rad * FRAME_BUFFER_WIDTH / screen_width;
  float scaledRadY = rad * FRAME_BUFFER_HEIGHT / screen_height;
  int scaledCenX = centreX * FRAME_BUFFER_WIDTH / screen_width;
  int scaledCenY = centreY * FRAME_BUFFER_HEIGHT / screen_height;

  // Loop over half the height of the circle in pixel size steps
  for (int y = 0; y <= scaledRadY; ++y) {
    float yscaled2 = float(y * screen_height / FRAME_BUFFER_HEIGHT) *
                     float(y * screen_height / FRAME_BUFFER_HEIGHT);
    float x_limit =
      std::sqrt(rad * rad - yscaled2) * FRAME_BUFFER_WIDTH / screen_width;
    float lineX = float(scaledCenX) + 0.5 - x_limit;
    float length = 2 * x_limit;
    // if (y == debugY) {
    //   debug1 = rad;
    //   debug2 = x_limit;
    //   debug3 = scaledRadY;
    // }
    drawAASpan(lineX, scaledCenY - y, x_limit, pen);
    if (y != 0) {
      drawAASpan(lineX, scaledCenY + y, x_limit, pen);
    }
  }

  // DEBUG: Draw centre pixel
  display->set_pen(display->create_pen(128, 128, 0));
  drawPixel(Point(scaledCenX, scaledCenY));
  // // Work out width of central line
  // float lineX = float(centreX - rad) * FRAME_BUFFER_WIDTH / screen_width;
  // float width = 2 * rad * FRAME_BUFFER_WIDTH / screen_width;

  // drawAASpan(lineX, centreY * FRAME_BUFFER_HEIGHT / screen_height, width, pen);
}

void drawCircle(int x, int y, int rad, uint16_t pen) {
  if (DRAW_AA) {
    drawCircleAA(x, y, rad, pen);
  } else {
    int rad_x, rad_y;
    Point position =
        Point(x * pixelMultiplier * FRAME_BUFFER_WIDTH / screen_width, y * pixelMultiplier * FRAME_BUFFER_HEIGHT / screen_height);

    display->set_pen(pen);

    if (rad == 1) {
      display->pixel(position);
    } else if (FRAME_BUFFER_WIDTH != FRAME_BUFFER_HEIGHT) {
      if (FRAME_BUFFER_WIDTH > FRAME_BUFFER_HEIGHT) {
        rad_x = rad;
        rad_y = rad * FRAME_BUFFER_HEIGHT / FRAME_BUFFER_WIDTH;
      } else {
        rad_x = rad * FRAME_BUFFER_WIDTH / FRAME_BUFFER_HEIGHT;
        rad_y = rad;
      }
      footlegGraphics->circle_scaled(position, rad_x, rad_y);
    } else {
      display->circle(position, rad * FRAME_BUFFER_WIDTH / screen_width);
    }
  }
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

  presto = new ST7701(
      FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, ROTATE_0,
      SPIPins{spi1, LCD_CS, LCD_CLK, LCD_DAT, PIN_UNUSED, LCD_DC, BACKLIGHT},
      back_buffer);
  display = new PicoGraphics_PenRGB565(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, front_buffer);

  footlegGraphics = new FootlegGraphics(display,FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, front_buffer);

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
              if (showText)
                debugY++;
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
              pixelMultiplier++;
              if (pixelMultiplier > 5) pixelMultiplier = 1;
              actionTaken = true;
              lastSettingsChange = time_us_64();
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

    int y = 100;
    int iX1 = 25;
    display->set_pen(RED);
    debug1 = RED;
    debug2 = front_buffer[y * FRAME_BUFFER_WIDTH + iX1];
    display->set_pixel(Point(iX1, y));
    debug3 = front_buffer[y * FRAME_BUFFER_WIDTH + iX1];

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
    int cx = screen_width / pixelMultiplier;
    int cy = screen_height / pixelMultiplier;
    drawCircle(cx/4, cy/4, 14, RED);
    drawCircle(cx/4, cy/2, 15, GREEN);
    drawCircle(cx/4, cy*3/4, 16, BLUE);
    drawCircle(cx/2, cy/4, 17, RED);
    drawCircle(cx/2, cy/2, 18, GREEN);
    drawCircle(cx/2, cy*3/4, 19, BLUE);
    drawCircle(cx*3/4, cy/4, 20, RED);
    drawCircle(cx*3/4, cy/2, 21, GREEN);
    drawCircle(cx*3/4, cy*3/4, 13, BLUE);

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
        sprintf(prefix, "WxH:%ix%i AA Touch:%i,%i PM:%i", FRAME_BUFFER_WIDTH,
                FRAME_BUFFER_HEIGHT, lastTouch.x, lastTouch.y, pixelMultiplier);
      } else {
        sprintf(prefix, "WxH:%ix%i Touch:%i,%i PM:%i", FRAME_BUFFER_WIDTH,
                FRAME_BUFFER_HEIGHT, lastTouch.x, lastTouch.y, pixelMultiplier);
      }

      //sprintf(suffix, " fps:%5.2f", fps);
      sprintf(suffix, " debug:%5.2f,%5.2f,%5.2f at %i", debug1,debug2,debug3,debugY);

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
      display->set_pixel(Point(4,4));
    }

    presto->update(display);
  }

  return 0;
}
