/*
 * An example project for the Pimoroni Presto with the sensor stick add-on.
 * The readings from the accelerometer are taken, and the resulting vector
 * rotated by an angle to reflect the plane of the screen. The angle is 
 * incremented each time a single-tap event is triggered (sensed by the 
 * accelerometer) to help calibrate the rotation angle needed so that X
 * is equal to 1G when the screen is vertical. This assumes the sensor stick 
 * is mounted on the base of the Presto parallel to the desk, using the
 * bracket I designed which can be found here:
 * https://www.printables.com/model/1322163-mounting-clip-for-the-sensor-stick-and-presto-from
 *
 * Copyright (c) 2025 Dr Footleg
 *
 * License: GNU GPL v3.0
 */

#include "../drivers/lsm6ds3/lsm6ds3.hpp"
#include "drivers/st7701/st7701.hpp"
#include "libraries/pico_graphics/pico_graphics.hpp"
#include "pico/multicore.h"
#include "pico/sync.h"

using namespace pimoroni;

// To get the best graphics quality without exceeding the RAM, use a width of
// 480 with a height of 240. All drawing commands will be done on a 480 x 480
// screen size, and scaled accordingly to draw round circles on the screen
// regardless of the buffer aspect ratio used.
#define FRAME_BUFFER_WIDTH 480
#define FRAME_BUFFER_HEIGHT 240

// This is the resolution of the simulation space (independent of the resolution
// of the frame buffer we decide to render it onto)
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
LSM6DS3* accel;

struct Vector3 {
  double x, y, z;
};

Vector3 rotateX(Vector3 v, double angle_deg) {
  double angle_rad = angle_deg * M_PI / 180.0;  // Convert degrees to radians

  double cos_theta = cos(angle_rad);
  double sin_theta = sin(angle_rad);

  Vector3 rotated;
  rotated.x = v.x;
  rotated.y = v.y * cos_theta - v.z * sin_theta;
  rotated.z = v.y * sin_theta + v.z * cos_theta;

  return rotated;
}

Vector3 rotateY(Vector3 v, double angle_deg) {
  double angle_rad = angle_deg * M_PI / 180.0;  // Convert degrees to radians

  double cos_theta = cos(angle_rad);
  double sin_theta = sin(angle_rad);

  Vector3 rotated;
  rotated.x = v.x * cos_theta + v.z * sin_theta;
  rotated.y = v.y;  // Y remains unchanged
  rotated.z = -v.x * sin_theta + v.z * cos_theta;

  return rotated;
}

int main() {
  set_sys_clock_khz(240000, true);
  stdio_init_all();

  gpio_init(LCD_CS);
  gpio_put(LCD_CS, 1);
  gpio_set_dir(LCD_CS, 1);

  presto = new ST7701(
      FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, ROTATE_0,
      SPIPins{spi1, LCD_CS, LCD_CLK, LCD_DAT, PIN_UNUSED, LCD_DC, BACKLIGHT},
      back_buffer);
  display = new PicoGraphics_PenRGB565(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT,
                                       front_buffer);

  presto->init();

  I2C i2c(40, 41);  // QWST port i2c on the Presto
  accel = new LSM6DS3(&i2c);

  Pen BG = display->create_pen(0, 0, 0);
  Pen WHITE = display->create_pen(255, 255, 255);
  Pen GREY = display->create_pen(96, 96, 96);
  Pen RED = display->create_pen(255, 0, 0);
  Pen ORANGE = display->create_pen(255, 128, 0);
  Pen YELLOW = display->create_pen(255, 255, 0);
  Pen GREEN = display->create_pen(0, 255, 0);
  Pen BLUE = display->create_pen(0, 0, 255);
  Pen PINK = display->create_pen(192, 0, 128);
  Pen PURPLE = display->create_pen(128, 0, 128);

  int text_x = 10;

  char msg[64];
  double angle = -56.0;  // -53.4
  static const int ACC1G = 17000; // Accelerometer reading for 1G

  while (true) {
    display->set_pen(BG);
    display->clear();

    // Read IMU
    LSM6DS3::SensorData acceldata = accel->getReadings();

    // Rotate vector into plane of presto screen
    Vector3 data =
        rotateY({(float)acceldata.ax / ACC1G, (float)acceldata.ay / ACC1G,
                 (float)acceldata.az / ACC1G},
                angle);

    sprintf(msg, "ax: %.2f ay:%.2f az:%.2f a:%.2f", data.x, data.y, data.z,
            angle);

    display->set_pen(PINK);
    display->rectangle({20, display->bounds.h / 2, display->bounds.w - 40,
                        display->bounds.h - 110});
    display->set_pen(YELLOW);
    Point text_location = {text_x, display->bounds.h / 2 + 20};
    display->text(msg, text_location, display->bounds.w - text_location.x);

    if (accel->singleTapDetected()) {
      angle += 0.2;
      text_x += 10;
      if (text_x > display->bounds.w / 2) text_x = 10;
    }

    presto->update(display);
    sleep_ms(10);
  }
}