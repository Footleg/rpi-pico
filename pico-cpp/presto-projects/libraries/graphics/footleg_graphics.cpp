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
#include "footleg_graphics.hpp"
#include "libraries/pico_graphics/pico_graphics.hpp"

using namespace pimoroni;

FootlegGraphics::FootlegGraphics(PicoGraphics_PenRGB565* display, uint16_t frame_buffer_width, uint16_t frame_buffer_height, uint16_t* screen_buffer) : 
  display(display), frame_buffer_width(frame_buffer_width), frame_buffer_height(frame_buffer_height), screen_buffer(screen_buffer) {};

void FootlegGraphics::circle_scaled(const Point &p, int32_t radius_x, int32_t radius_y) 
    {
    // Iterate through the height to draw ellipse (scaled circle)
    for (int y = -radius_y; y <= radius_y; ++y) {
        double y_scaled = y / double(radius_y);  // Scaling y-coordinate
        double x_limit = double(radius_x) * std::sqrt(1 - y_scaled * y_scaled);
        int start_x = p.x - static_cast<int>(x_limit);
        int length = 2 * static_cast<int>(x_limit);
        display->pixel_span(Point(start_x, p.y + y), length);
    }
}

void FootlegGraphics::drawPixelSpan(Point p, int width) {
    if(width > 0) display->pixel_span(p, width);
}
  
void FootlegGraphics::drawAASpan(float x, int y, float width, uint16_t pen) {
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
    uint16_t penAA = display->create_pen(rgb.r * spanX, rgb.g * spanX, rgb.b * spanX);
    display->set_pen(penAA);
    if (screen_buffer[y * frame_buffer_width + iX1] == 0) display->set_pixel(Point(iX1, y));
    if (screen_buffer[y * frame_buffer_width + iX2] == 0) display->set_pixel(Point(iX2, y));

    // Draw solid span between end pixels
    display->set_pen(pen);
    drawPixelSpan(Point(iX1 + 1, y), iX2 - iX1 - 1);
  } else {
    // Solid line, so include end pixels in span
    display->set_pen(pen);
    drawPixelSpan(Point(iX1, y), std::round(width * 2));
  }
}
  
void FootlegGraphics::drawCircleAA(int centreX, int centreY, int rad, uint16_t pen) {
  // Convert radius to pixel scale
  float scaledRadX = rad * frame_buffer_width / screen_width;
  float scaledRadY = rad * frame_buffer_height / screen_height;
  int scaledCenX = centreX * frame_buffer_width / screen_width;
  int scaledCenY = centreY * frame_buffer_height / screen_height;

  // Loop over half the height of the circle in pixel size steps
  for (int y = 0; y <= scaledRadY; ++y) {
    float yscaled2 = float(y * screen_height / frame_buffer_height) *
                      float(y * screen_height / frame_buffer_height);
    float x_limit =
      std::sqrt(rad * rad - yscaled2) * frame_buffer_width / screen_width;
    float lineX = float(scaledCenX) + 0.5 - x_limit;
    float length = 2 * x_limit;
    drawAASpan(lineX, scaledCenY - y, x_limit, pen);
    if (y != 0) {
      drawAASpan(lineX, scaledCenY + y, x_limit, pen);
    }
  }
}

void FootlegGraphics::drawCircle(int x, int y, int rad, uint16_t pen) {
  int rad_x, rad_y;
  Point position =
      Point(x * frame_buffer_width / screen_width, y * frame_buffer_height / screen_height);

  display->set_pen(pen);

  if (rad == 1) {
    display->pixel(position);
  } else if (frame_buffer_width != frame_buffer_height) {
    if (frame_buffer_width > frame_buffer_height) {
      rad_x = rad;
      rad_y = rad * frame_buffer_height / frame_buffer_width;
    } else {
      rad_x = rad * frame_buffer_width / frame_buffer_height;
      rad_y = rad;
    }
    circle_scaled(position, rad_x, rad_y);
  } 
  else {
    display->circle(position, rad * frame_buffer_width / screen_width);
  }
}

