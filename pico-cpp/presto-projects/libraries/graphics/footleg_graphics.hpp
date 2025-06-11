/*
 * A graphics library to draw anti-aliased edged circles on displays with
 * non-square pixels. Written for the Pimoroni Presto display to allow
 * double buffering with resolutions of 240 x 480 and 480 x 240 so that
 * the buffers fit into available memory. Circles can be drawn with
 * double width or double height pixels so they appear round when the
 * graphics buffer is stretched to the full screen resolution.
 *
 * Copyright (c) 2025 Dr Footleg
 *
 * License: GNU GPL v3.0
 */
#include "libraries/pico_graphics/pico_graphics.hpp"

using namespace pimoroni;

class FootlegGraphics {
 public:
  FootlegGraphics(PicoGraphics_PenRGB565* display, uint16_t* screen_buffer);
  void circle_scaled(const Point& p, int32_t radius_x, int32_t radius_y);
  void drawAASpan(float x, int y, float width, uint16_t pen);
  void drawCircleAA(int centreX, int centreY, int rad, uint16_t pen);
  void drawCircle(int x, int y, int rad, uint16_t pen);

 private:
  PicoGraphics_PenRGB565* display;
  uint16_t* screen_buffer;
  uint16_t screen_width = 480;
  uint16_t screen_height = 480;
  void drawPixelSpan(Point p, int width);
};
