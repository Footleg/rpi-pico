/*
 * A bouncing balls simulation which started by me adding collisions to the
 * Pimoroni balls demo, but then I added attractive/repulsive forces as an
 * option and it became a fun simulation of spheres interacting. Ported from my
 * tufty2040 example to the Pimoroni Presto.
 *
 * Copyright (c) 2025 Dr Footleg
 *
 * License: GNU GPL v3.0
 */
#include <math.h>
#include <string.h>

#include <cstdlib>
#include <vector>

#include "../drivers/lsm6ds3/lsm6ds3.hpp"
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

using namespace pimoroni;

// To get the best graphics quality without exceeding the RAM, use a width of
// 480 with a height of 240 Calculations will be done on a 480 x 480 screen
// size, and scaled accordingly to draw round balls
#define FRAME_BUFFER_WIDTH 480
#define FRAME_BUFFER_HEIGHT 240

bool DRAW_AA = true;
static const int MAX_BALLS = 255;  // Limit of the vector? Crashes above 256
                                   // possibly to due running out of RAM?

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

static const uint MAXBALLSIZE = 40;

static const int ACC1G = 17000; // Accelerometer reading for 1G
static const float gFactor = 0.2; // Scale force to apply for gravity
float friction = 0.02; // Dampening factor where 0.0 = no energy lost from system

// Maximum time in ms for a touch and release to be acted on as a 'short press'
static const uint TOUCH_SHORT_PRESS_TIME = 200;
// Time in ms after which a touch is acted on as a 'held action'
static const uint TOUCH_HELD_TIME = 1000;
// Times between the above values will trigger 'long press' actions

// Height and Width in pixels of the areas treated as corner presses on the
// touch screen
static const uint TOUCH_CORNER_SIZE = 60;

uint16_t back_buffer[FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT];
uint16_t front_buffer[FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT];

ST7701* presto;
PicoGraphics_PenRGB565* display;
FootlegGraphics* footlegGraphics;
LSM6DS3* accel;

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

struct idxPair {
  uint16_t idx1;
  uint16_t idx2;
};

pt createShape(int x = -999, int y = -999, int minX = 0, int minY = 0,
               int maxX = screen_width, int maxY = screen_height) {
  pt shape;

  if (x == -999 && y == -999) {
    shape.x = rand() % (minX + screen_width * (maxX - minX) / screen_width);
    shape.y = rand() % (minY + screen_height * (maxY - minY) / screen_height);
  } else {
    shape.x = minX + x * (maxX - minX) / screen_width;
    shape.y = minY + y * (maxY - minY) / screen_height;
  }
  shape.r = (rand() % (MAXBALLSIZE - 2)) + 2;
  shape.dx = 4.0 - (float(rand() % 255) / 32.0);
  shape.dy = 4.0 - (float(rand() % 255) / 32.0);
  // Generate random colour which is not too dark
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  while (r + g + b < 224) {
    r = rand() % 255;
    g = rand() % 255;
    b = rand() % 255;
  }
  shape.pen = display->create_pen(r, g, b);
  return shape;
};

// float debug1, debug2, debug3 = 0.0;

struct Vector3 {
  double x, y, z;
};

Vector3 rotateY(Vector3 v) {
  double angle_rad = -0.932; // Angle of Presto screen to base in radians

  double cos_theta = cos(angle_rad);
  double sin_theta = sin(angle_rad);

  Vector3 rotated;
  rotated.x = v.x * cos_theta + v.z * sin_theta;
  rotated.y = v.y;  // Y remains unchanged
  rotated.z = -v.x * sin_theta + v.z * cos_theta;

  return rotated;
}

int main() {
  char msg[256];  // Make sure buffer for text messages doesn't overflow
  uint16_t frame_counter, lastFC = 0;
  uint64_t start_fps, elapsed, lastSettingsChange;  // Microsecond times
  double fps = 0.0f, prevFps = 0.0f;  // Frames per second to display

  // Initialise random numbers seed using floating adc input reading
  adc_init();
  // Make sure GPIO is high-impedance, no pullups etc
  adc_gpio_init(47);
  // Select ADC input 7 (GPIO47) which is available on the Tufty2040 board
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
  display = new PicoGraphics_PenRGB565(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT,
                                       front_buffer);
  footlegGraphics = new FootlegGraphics(display, FRAME_BUFFER_WIDTH,
                                        FRAME_BUFFER_HEIGHT, front_buffer);
  presto->init();

  static I2C i2c(30, 31, 100000);
  static TouchScreen touch(&i2c);
  
  static I2C i2c_qwst(40, 41);
  accel = new LSM6DS3(&i2c_qwst);
  LSM6DS3::SensorData acceldata;

  Point text_location(5, 5);
  Point lastTouch(0, 0);

  Pen BG = display->create_pen(0, 0, 0);
  Pen WHITE = display->create_pen(255, 255, 255);

  // Get the start time (used to calculate fps)
  start_fps = time_us_64();
  lastSettingsChange = start_fps;

  std::vector<pt> shapes;          // These are the balls in the simulation
  std::vector<idxPair> mergeList;  // List of pairs of balls indices to be
                                   // merged into one after collisions

  // Create 2 balls initially
  for (int i = 0; i < 1; i++) {  // DEBUG: Creating 25
    shapes.push_back(createShape());
  }

  // Simulation boundaries
  float minX = 0;
  float minY = 0;
  float maxX = screen_width;
  float maxY = screen_height;
  uint8_t renderSkip = 0;
  uint8_t renderCount = 0;

  uint8_t mode = MODE_BOUNCE;
  bool showText = true;
  float forcePower = -4.0f;
  bool gravity = true;
  bool mass = true;
  bool mergesOn = false;
  Vector3 dataG;
  float dampening = 1.0;

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
          // Use position of touch to set force if in force mode
          if (mode == MODE_FORCES) {
            forcePower =
                pow(((float)touchPoint.y - touch.bounds.h / 2) / 16.0f, 2);
            if (touchPoint.y < (touch.bounds.h / 2)) forcePower *= -1;
            lastSettingsChange = time_us_64();
          } else if (gravity) {
            // Set dampening factor
            friction = 0.08 * (float)(touch.bounds.h - 10 - touchPoint.y) / touch.bounds.h;
            if (friction < 0) friction *= 4; // A bit of fun, the very bottom of the screen edge allows -ve friction
          }
        } else {
          // Create balls at position of touch (until released)
          if (shapes.size() < MAX_BALLS) {
            shapes.push_back(createShape(touchPoint.x, touchPoint.y, minX, minY,
                                         maxX, maxY));
          }
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
              if (showText)
                DRAW_AA =
                    !DRAW_AA;  // Toggle AA off on alternate showing of text
            } else if (touchPoint.y > touch.bounds.h - TOUCH_CORNER_SIZE) {
              // Bottom Left Corner
              if (mode == MODE_BOUNCE) {
                // Toggle mass if in bounce mode
                mass = !mass;
                // Toggle gravity on alternate toggles
                if (mass) {
                  gravity = !gravity;
                }
                actionTaken = true;
                lastSettingsChange = time_us_64();
              } else {
                // Toggle merges if in force mode
                mergesOn = !mergesOn;
                actionTaken = true;
                lastSettingsChange = time_us_64();
              }
            }
          } else if (touchPoint.x > touch.bounds.w - TOUCH_CORNER_SIZE) {
            // Right side of screen
            if (touchPoint.y < TOUCH_CORNER_SIZE) {
              // Top Right Corner
              // Find current bounds shink area to just include them
              float minXc = shapes[0].x;
              float minYc = shapes[0].y;
              float maxXc = minXc;
              float maxYc = minYc;
              for (auto& shape : shapes) {
                if (shape.x < minXc) minXc = shape.x;
                if (shape.x > maxXc) maxXc = shape.x;
                if (shape.y < minYc) minYc = shape.y;
                if (shape.y > maxYc) maxYc = shape.y;
              }
              float midX = minXc + (maxXc - minXc) / 2;
              float midY = minYc + (maxYc - minYc) / 2;
              float totX = maxXc - minXc;
              float totY = maxYc - minYc;
              if (totX > totY * screen_width / screen_height) {
                totY = totX * screen_height / screen_width;
              } else {
                totX = totY * screen_width / screen_height;
              }
              minX = midX - totX / 1.95;
              maxX = midX + totX / 1.95;
              minY = midY - totY / 1.95;
              maxY = midY + totY / 1.95;

              if (renderSkip > 0) renderSkip--;

              if (maxX - minX < screen_width) {
                // Reset to 1:1 scale
                minX = 0;
                minY = 0;
                maxX = screen_width;
                maxY = screen_height;
                renderSkip = 1;
              }
              actionTaken = true;
            } else if (touchPoint.y > touch.bounds.h - TOUCH_CORNER_SIZE) {
              // Bottom Right Corner
              // Find current bounds and multiply by 1.5
              float minXc = shapes[0].x;
              float minYc = shapes[0].y;
              float maxXc = minXc;
              float maxYc = minYc;
              for (auto& shape : shapes) {
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

              actionTaken = true;
            }
          }

          if (actionTaken == false) {
            // Press/release was not in any special screen area.
            if (checkBtn < TOUCH_SHORT_PRESS_TIME) {
              // Very short touch (under 200ms)
              if (shapes.size() < MAX_BALLS) {
                // Create a ball at position of touch
                // Convert touch position to simulation area space
                shapes.push_back(createShape(touchPoint.x, touchPoint.y, minX,
                                             minY, maxX, maxY));
              }
            } else {
              // Long press anywhere but the screen corners
              // Update the simulation mode.
              mode++;
              if (mode > 1) mode = 0;
              lastSettingsChange = time_us_64();
            }
          }
        }
      }
    }

    if(gravity) {
      acceldata = accel->getReadings();
      // Rotate vector into plane of presto screen
      dataG = rotateY({(float)acceldata.ax / ACC1G, (float)acceldata.ay / ACC1G, (float)acceldata.az / ACC1G});
    }

    uint16_t i = 0;  // Index of current shape in loop over all shapes
    if (friction > 0 && friction < 0.0008) {
      dampening = 1.0; // Treat close to zero as zero
    } else {
      dampening = 1.0 - friction/10;
    }
    
    for (auto& shape : shapes) {
      if (shape.r > 0) {
        // Apply gravity if on
        if (gravity) {
          // ax +ve is up the screen
          // ay +ve is to left side of screen
          shape.dx -= dataG.y * gFactor;
          shape.dy -= dataG.x * gFactor;
          shape.dx *= dampening;
          shape.dy *= dampening;
        }

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
                if (mode == MODE_FORCES && mergesOn) {
                  // Add shapes to merge list
                  idxPair pair;
                  pair.idx1 = i;
                  pair.idx2 = j;
                  mergeList.push_back(pair);
                } else {
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
        i++;
      }
    }  // End position and velocities update loop

    if (mergesOn && mergeList.size() > 0) {
      // Merge balls in merge list
      for (uint16_t i = 0; i < mergeList.size(); i++) {
        // Item 2 will be merged with item 1. Then remove item 2 from simulation
        idxPair& pair = mergeList.at(i);
        pt& primeBall = shapes.at(pair.idx1);
        pt& secBall = shapes.at(pair.idx2);
        primeBall.x = (primeBall.x + secBall.x) / 2;
        primeBall.y = (primeBall.y + secBall.y) / 2;
        if (mass) {
          // Combine velocity with radius
          primeBall.dx = (primeBall.dx * primeBall.r + secBall.dx * secBall.r) /
                         (primeBall.r + secBall.r);
          primeBall.dy = (primeBall.dy * primeBall.r + secBall.dy * secBall.r) /
                         (primeBall.r + secBall.r);
        } else {
          // Just add velocities
          primeBall.dx = (primeBall.dx + secBall.dx);
          primeBall.dy = (primeBall.dy + secBall.dy);
        }
        // double area1 = M_PI * std::pow(primeBall.r, 2);
        // double area2 = M_PI * std::pow(secBall.r, 2);
        primeBall.r =
            std::sqrt(std::pow(primeBall.r, 2) + std::pow(secBall.r, 2));
        secBall.r = 0;  // Flag for destruction, but don't delete yet or all our
                        // merge list indices will be affected

        sprintf(msg, "MergeLSz:%i b1:%i,r%i b2:%i,r%i", mergeList.size(),
                pair.idx1, primeBall.r, pair.idx2, secBall.r);
        // Item 2 is gone, so need to update all references to item 2  in the
        // rest of the merge list with item 1
        for (uint16_t j = i + 1; j < mergeList.size(); j++) {
          idxPair& testPair = mergeList.at(j);
          if (testPair.idx1 == pair.idx2) {
            testPair.idx1 = pair.idx1;
          }
          if (testPair.idx2 == pair.idx2) {
            testPair.idx2 = pair.idx1;
          }
        }
      }

      // All merges completed, now remove dead balls
      for (int16_t i = (shapes.size() - 1); i >= 0; i--) {
        pt& testBall = shapes.at(i);
        if (testBall.r == 0) {
          shapes.erase(shapes.begin() + i);
        }
      }

      // Wipe merge list
      mergeList.clear();
    }  // End merges block

    if (renderCount == 0) {
      // Update screen
      display->set_pen(BG);
      display->clear();

      for (auto& shape : shapes) {
        // Skip the slow calcs if 1:1 scale with screen
        int x, y, r;
        if (minX == 0 && minY == 0) {
          // Draw circles at 1:1 scale on screen
          x = shape.x;
          y = shape.y;
          r = shape.r;
        } else {
          // Draw circles scaled to boundaries
          x = screen_width * (shape.x - minX) / (maxX - minX);
          y = screen_height * (shape.y - minY) / (maxY - minY);
          r = screen_height * shape.r / (maxY - minY);
          if (r < 2) r = 2;
        }
        if (DRAW_AA) {
          footlegGraphics->drawCircleAA(x, y, r, shape.pen);
        } else {
          footlegGraphics->drawCircle(x, y, r, shape.pen);
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

      if (showText || elapsed - lastSettingsChange < 2000000) {
        // Update text for information shown on screen
        if (mergeList.size() == 0) {
          char suffix[128];
          // lastTouch = touch.last_touched_point();
          // sprintf(suffix, " Balls:%i fps:%5.2f Touch:%i,%i", shapes.size(),
          // fps,
          //         lastTouch.x, lastTouch.y);
          // sprintf(suffix, " Balls:%i fps:%5.2f debug:%5.2f,%5.2f,%5.2f",
          //         shapes.size(), fps, debug1, debug2, debug3);

          sprintf(suffix, " Balls:%i Friction: %.2f fps:%.2f", shapes.size(), friction * 12.5, fps);

          switch (mode) {
            case MODE_BOUNCE:
              sprintf(msg, "Bounce");
              break;
            case MODE_FORCES:
              sprintf(msg, "Force %.1f", forcePower);
              break;
            default:
              sprintf(msg, "Unsupported Mode!");
          }
          // Append flag states to message buffer
          if (mass || mergesOn || gravity) {
            strcat(msg, "(");
          }
          if (mass) {
            strcat(msg, "m");
          }
          if (mergesOn) {
            strcat(msg, "c");
          }
          if (gravity) {
            strcat(msg, "g");
          }
          if (mass || mergesOn || gravity) {
            strcat(msg, ")");
          }
          if (DRAW_AA) {
            strcat(msg, " AA");
          }

          // Concatenate the contents of the second array into the combined
          // array
          strcat(msg, suffix);
        }

        // Render Mode info and FPS to screen
        display->set_pen(WHITE);
        display->text(msg, text_location, display->bounds.w - text_location.x,
                      2);
      }

      presto->update(display);
    }

    // Increment render counter (graphics are only rendered on loop cycles where
    // this rolls over to zero)
    renderCount++;
    if (renderCount > renderSkip) renderCount = 0;
  }

  return 0;
}
