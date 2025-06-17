/*
 * A demonstration project of my RGB animations library using the Pimoroni
 * Presto. A virtual LED matrix of variable resolutions is drawn on the screen
 * as circles. The size of the virtual LED matrix is controlled by touching the
 * top right or bottom right of the screen to set LED size. Press and hold on
 * the screen anywhere to toggle residual mode. Show/hide the text by touching
 * the top left of the screen. Switch animations by touching the bottom left.
 *
 * Supports drawing at full screen resolution 480 x 480 by not using double
 * buffer. There is not enough RAM to support a double buffer of 480 x 480
 * unless we use the much slower PSRAM (which still shows flicker due to the
 * refresh rate being so slow).
 *
 * Copyright (c) 2025 Dr Footleg
 *
 * License: GNU GPL v3.0
 */
#include <math.h>
#include <string.h>

#include <cstdlib>

#include "../drivers/lsm6ds3/lsm6ds3.hpp"
#include "../drivers/touchscreen/touchscreen.hpp"
#include "../libraries/graphics/footleg_graphics.hpp"
#include "crawler.h"  //This is one of the animation classes used to generate output for the display
#include "drivers/st7701/st7701.hpp"
#include "golife.h"  //This is one of the animation classes used to generate output for the display
#include "gravityparticles.h"  //Another animation class used to generate output for the display
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

// To get the best graphics quality without exceeding the RAM, this example
// uses a screen resolution of 480 x 480 without a double buffer.
// Tearing is avoided by drawing cells directly to the screen or clearing
// them by drawing over them in black, rather than clearing the whole screen
// and redrawing it for each frame.
#define FRAME_BUFFER_WIDTH 480
#define FRAME_BUFFER_HEIGHT 480

#define DRAW_BUF_SIZE \
  (FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT * sizeof(uint16_t))

static const uint BACKLIGHT = 45;
static const uint LCD_CLK = 26;
static const uint LCD_CS = 28;
static const uint LCD_DAT = 27;
static const uint LCD_DC = -1;
static const uint LCD_D0 = 1;

// Use one of these 3 buffer options. If using the psram buffer, then uncomment
// the allocation line at the start of the main() function
uint16_t screen_buffer[FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT];
// uint16_t draw_buffer[FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT]; // Dbl buffer
// in main RAM
uint16_t* draw_buffer = screen_buffer;  // Single buffer (draw buffer mapped
                                        // directly to screen buffer RAM)
// uint16_t* draw_buffer; // Double buffer using PSRAM. Will be allocated using
// psram in main()

ST7701* presto;
PicoGraphics_PenRGB565* display;
LSM6DS3* accel;

Pen BG;  // Set in main after display object has been created, but declared here
         // so globally accessible

// Maximum time in ms for a touch and release to be acted on as a 'short press'
static const uint TOUCH_SHORT_PRESS_TIME = 200;
// Time in ms after which a touch is acted on as a 'held action'
static const uint TOUCH_HELD_TIME = 1000;
// Times between the above values will trigger 'long press' actions

// Height and Width in pixels of the areas treated as corner presses on the
// touch screen
static const uint TOUCH_CORNER_SIZE = 60;

static const int ACC1G = 17000;      // Accelerometer reading for 1G
static const float gFactor = 0.2;    // Scale force to apply for gravity
static const float friction = 0.99;  // Dampening factor (represents friction)

bool showText = true;

// Animations settings
static const uint8_t animModeGol = 0;
static const uint8_t animModeCrawler = 1;
static const uint8_t animModeParticles = 2;

uint16_t steps = 10;
uint16_t minSteps = 2;

uint8_t golFadeSteps = 0;
uint16_t golDelay = 100;
uint8_t golStartPattern = 0;

bool residual = true;
uint16_t pixelsRedrawn = 0;

// RGB Matrix class which passes itself as a renderer implementation into the
// animation class. Needs to be declared before the global variable accesses it
// in this file. Passed as a reference into the animation class, so need to
// dereference 'this' which is a pointer using the syntax *this
class Animation : public RGBMatrixRenderer {
 public:
  Animation(uint8_t pixScale, uint16_t steps_, uint16_t minSteps_,
            uint8_t golFadeSteps_, uint16_t golDelay_, uint8_t golStartPattern_,
            uint16_t shake, uint8_t bounce_)
      : RGBMatrixRenderer{static_cast<uint16_t>(FRAME_BUFFER_WIDTH /
                                                ceil(pixScale * 1.2)),
                          static_cast<uint16_t>(FRAME_BUFFER_HEIGHT /
                                                ceil(pixScale * 1.2))},
        animCrawler(*this, steps_, minSteps_, false),
        animGol(*this, golFadeSteps_, golDelay_, 0, golStartPattern_),
        animParticles(*this, shake, bounce_),
        pixelSize(pixScale) {
    footlegGraphics = new FootlegGraphics(display, draw_buffer);

    // Clear screen
    display->set_pen(BG);
    display->clear();

    // Precalculate pixel radius
    rad = pixelSize / 2;  // - 1
    if (rad < 1) rad = 1;

    // Initialise mode
    aniMode = animModeGol;
    cycles = 0;
  }

  virtual ~Animation() {
    // animGol.~GameOfLife();
    // animCrawler.~Crawler();
    // animParticles.~GravityParticles();
  }

  void animationStep() {
    // (Don't clear screen as we are not using double buffer so we just draw
    // over pixels as they need updating)
    // display->set_pen(BG);
    // display->clear();

    switch (aniMode) {
      case animModeGol:
        animGol.runCycle();
        break;
      case animModeCrawler:
        animCrawler.runCycle();
        sleep_ms(1);
        break;
      case animModeParticles:
        animParticles.runCycle();
        if (cycles > 1000) {
          cycles = 0;
          animParticles.setAcceleration(100 - rand() % 200, 100 - rand() % 200);
        }
        sleep_ms(1);
        break;
    }
    cycles++;
  }

  virtual void showPixels() { presto->update(display); }

  virtual void outputMessage(char msg[]) {
    if (showText) {
      // Blank rectangle area of one line of text across top of screen
      display->set_pen(BG);
      display->rectangle({0, 0, 480, 12});
      // Draw text in blanked area
      display->set_pen(WHITE);
      Point text_location(0, 0);
      display->text(msg, text_location, display->bounds.w);
    }
  }

  virtual void msSleep(int delay_ms) { sleep_ms(delay_ms); }

  virtual int16_t random_int16(int16_t a, int16_t b) {
    return a + rand() % (b - a);
  }

  void setMode(uint8_t mode) {
    cycles = 0;
    aniMode = mode;
  }

  void setCrawlerMode(bool mode) { animCrawler.anyAngle = mode; }

  void setParticles() {
    // Set particles
    animParticles.setAcceleration(0, 150);

    animParticles.clearParticles();
    animParticles.imgToParticles();
  }

  uint16_t particleCount() { return animParticles.getParticleCount(); }

  void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    // Get smaller and larger points
    uint16_t xa = x1;
    uint16_t xb = x2;
    uint16_t ya = y1;
    uint16_t yb = y2;
    if (x2 < x1) {
      xa = x2;
      xb = x1;
      ya = y2;
      yb = y1;
    }

    RGB_colour yellow = {255, 200, 120};
    RGB_colour red = {255, 0, 0};
    RGB_colour blue = {0, 0, 255};

    if (xb - xa == 0) {
      if (y2 > y1) {
        ya = y1;
        yb = y2;
      } else {
        ya = y2;
        yb = y1;
      }
      for (uint16_t y = ya; y <= yb; y++) {
        setPixelColour(xa, y, yellow);
      }
    } else {
      for (uint16_t x = xa; x <= xb; x++) {
        uint16_t y = ya + (yb - ya) * (x - xa) / (xb - xa);
        setPixelColour(x, y, yellow);
      }
      // Mark ends
      setPixelColour(x1, y1, red);
      setPixelColour(x2, y2, blue);
    }

    updateDisplay();
  }

  uint8_t rad;

 private:
  Pen WHITE = display->create_pen(255, 255, 255);
  Pen GREY = display->create_pen(96, 96, 96);
  Pen RED = display->create_pen(255, 0, 0);
  Pen ORANGE = display->create_pen(255, 128, 0);
  Pen YELLOW = display->create_pen(255, 255, 0);
  Pen GREEN = display->create_pen(0, 255, 0);
  Pen BLUE = display->create_pen(0, 0, 255);
  Pen PINK = display->create_pen(192, 0, 128);
  Pen PURPLE = display->create_pen(128, 0, 128);
  Crawler animCrawler;
  GameOfLife animGol;
  GravityParticles animParticles;
  uint8_t aniMode;
  uint16_t cycles;
  uint8_t pixelSize;
  FootlegGraphics* footlegGraphics;

  virtual void setPixel(uint16_t x, uint16_t y, RGB_colour colour) {
    if (!showText || y > floor(12 / (pixelSize + 1))) {
      pixelsRedrawn++;
      Pen pen = display->create_pen(colour.r, colour.g, colour.b);
      if (pixelSize == 0) {
        // Not using this for single pixels now, as not enough RAM to handle 1:1
        // pixel mapping on screen. This would only use 1/4 of the screen.
        // Instead for pixel size of 1 we draw alternate pixels as cells.
        display->set_pen(pen);
        display->set_pixel(Point(x, y));
      } else if (pixelSize < 3) {
        uint16_t scrnX = x * ceil(pixelSize * 1.2);
        uint16_t scrnY = y * ceil(pixelSize * 1.2);
        if (residual &&
            (aniMode != animModeGol || animGol.getIteration() > 4)) {
          if (pen > 0) {
            // Draw surround to enables trails in non=GOL modes
            if (aniMode != animModeGol) {
              RGB rgb = PicoGraphics::rgb565_to_rgb(pen);
              Pen faintPen =
                  display->create_pen(rgb.r / 3, rgb.g / 3, rgb.b / 3);
              footlegGraphics->drawCircleAA(scrnX, scrnY, pixelSize, faintPen);
            }
            display->set_pen(pen);
            // Draw square pixel
            display->rectangle({scrnX, scrnY, pixelSize, pixelSize});
          } else {
            RGB_colour rgb = animGol.getCellColour(x, y);
            Pen erase = display->create_pen(rgb.r / 3, rgb.g / 3, rgb.b / 3);
            // Draw surround
            display->set_pen(erase);
            display->rectangle(
                {scrnX - 1, scrnY - 1, pixelSize + 2, pixelSize + 2});
          }
        } else {
          display->set_pen(pen);
          if (pen == 0) {
            // Clear pixel and surround
            display->rectangle(
                {scrnX - 1, scrnY - 1, pixelSize + 2, pixelSize + 2});
          } else {
            // Draw square pixel
            display->rectangle({scrnX, scrnY, pixelSize, pixelSize});
          }
        }
      } else {
        uint16_t scrnX = (x * ceil(pixelSize * 1.2) + ceil(pixelSize * 0.6)) *
                         (480 / display->bounds.w);
        uint16_t scrnY = (y * ceil(pixelSize * 1.2) + ceil(pixelSize * 0.6)) *
                         (480 / display->bounds.h);
        if (residual &&
            (aniMode != animModeGol || animGol.getIteration() > 4)) {
          Pen erase =
              display->create_pen(colour.r / 3, colour.g / 3, colour.b / 3);
          if (pen > 0) {
            // Draw larger faint colour surround for cell to create residual
            // colour
            footlegGraphics->drawCircle(
                scrnX, scrnY, (rad + 2) * (480 / display->bounds.w), erase);
            // Draw actual cell
            footlegGraphics->drawCircleAA(scrnX, scrnY,
                                          rad * (480 / display->bounds.w), pen);
          } else {
            // Wipe centre of cell with residual colour from GOL cells array
            RGB_colour rgb = animGol.getCellColour(x, y);
            erase = display->create_pen(rgb.r / 3, rgb.g / 3, rgb.b / 3);
            footlegGraphics->drawCircle(scrnX, scrnY,
                                        rad * (480 / display->bounds.w), erase);
          }
        } else {
          if (pen == 0) {
            // Clear pixel with larger circle
            footlegGraphics->drawCircle(
                scrnX, scrnY, (rad + 2) * (480 / display->bounds.w), BG);
          } else {
            footlegGraphics->drawCircleAA(scrnX, scrnY,
                                          rad * (480 / display->bounds.w), pen);
          }
        }
      }
    }
  }
};

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

  // Set up psram and allocate drawing buffer using this
  sfe_setup_psram(47);
  sfe_pico_alloc_init();
  // draw_buffer = (uint16_t*)malloc(DRAW_BUF_SIZE);

  presto = new ST7701(
      FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, ROTATE_0,
      SPIPins{spi1, LCD_CS, LCD_CLK, LCD_DAT, PIN_UNUSED, LCD_DC, BACKLIGHT},
      screen_buffer);
  display = new PicoGraphics_PenRGB565(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT,
                                       draw_buffer);

  presto->init();

  BG = display->create_pen(0, 0, 0);

  static I2C i2c(30, 31, 100000);
  static TouchScreen touch(&i2c);

  static I2C i2c_qwst(40, 41);
  accel = new LSM6DS3(&i2c_qwst);
  LSM6DS3::SensorData acceldata;

  // Set up animation vars
  uint16_t shake = 100;
  uint8_t bounce = 200;
  uint8_t pixelSize = 5;
  bool crawlerAnyAngle = false;
  uint8_t animationMode = animModeGol;
  uint8_t oldPixelSize;
  uint8_t oldFadeSteps;
  uint8_t oldGolStartPattern;

  Animation animation(pixelSize, steps, minSteps, golFadeSteps, golDelay,
                      golStartPattern, shake, bounce);

  Point text_location(5, 5);
  Point lastTouch(0, 0);

  // Get the start time (used to calculate fps)
  start_fps = time_us_64();
  lastSettingsChange = start_fps;

  // Main simulation loop
  while (true) {
    pixelsRedrawn = 0;

    // Call animation class method to run a cycle
    animation.animationStep();

    // Store config options so we can detect if any changed
    oldPixelSize = pixelSize;
    oldFadeSteps = golFadeSteps;
    oldGolStartPattern = golStartPattern;

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
              animationMode++;
              if (animationMode > 2) animationMode = 0;
              animation.setMode(animationMode);

              // Clear all pixels or convert to particles
              if (animationMode == animModeParticles) {
                animation.setParticles();
              } else {
                animation.clearImage();
              }
              animation.updateDisplay();
              actionTaken = true;
              lastSettingsChange = time_us_64();
            }
          } else if (touchPoint.x > touch.bounds.w - TOUCH_CORNER_SIZE) {
            // Right side of screen
            if (touchPoint.y < TOUCH_CORNER_SIZE) {
              // Top Right Corner
              if (pixelSize < 40) pixelSize++;
              actionTaken = true;
              lastSettingsChange = time_us_64();
            } else if (touchPoint.y > touch.bounds.h - TOUCH_CORNER_SIZE) {
              // Bottom Right Corner
              uint8_t minSize = 1;
              if (animationMode == animModeGol and golStartPattern > 0)
                minSize = 2;
              if (pixelSize > minSize) pixelSize--;
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
              // Toggle residual graphics feature
              residual = !residual;
              if (residual) {
                animation.residual = 50;
              } else {
                animation.residual = 0;
              }
              display->set_pen(BG);
              display->clear();
              lastSettingsChange = time_us_64();
            }
          }
        }
      }
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
    sprintf(prefix, " WxH:%ix%i ", display->bounds.w, display->bounds.h);

    sprintf(suffix, "size:%i pd:%i fps:%5.2f", pixelSize, pixelsRedrawn, fps);

    // Copy the contents of the first array into the combined array
    strcpy(msg, prefix);
    // Concatenate the contents of the second array into the combined
    // array
    strcat(msg, suffix);

    // Render Mode info and FPS to screen (need to do this inside animation
    // class before refresh is called)
    animation.outputMessage(msg);
    animation.showPixels();

    if (pixelSize != oldPixelSize || golFadeSteps != oldFadeSteps ||
        golStartPattern != oldGolStartPattern) {
      // Need to destroy and recreate animation objects
      animation.~Animation();
      animationMode = animModeGol;
      if (golStartPattern > 0 && golStartPattern != 3) {
        if (pixelSize > 8) pixelSize = 8;
        if (pixelSize < 2) pixelSize = 2;
        new (&animation) Animation(pixelSize, steps, minSteps, golFadeSteps,
                                   golDelay, golStartPattern, shake, bounce);
      } else if (pixelSize > 1) {
        new (&animation) Animation(pixelSize, steps, minSteps, golFadeSteps,
                                   golDelay, golStartPattern, shake, bounce);
      } else {
        new (&animation) Animation(1, steps, minSteps, golFadeSteps, golDelay,
                                   golStartPattern, shake, bounce);
      }
      // Reset residual value as animation class was recreated
      if (residual) {
        animation.residual = 50;
      } else {
        animation.residual = 0;
      }
    }
  }

  return 0;
}
