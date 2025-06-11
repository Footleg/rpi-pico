/*
 * A boilerplate template project for the Pimoroni Presto. Demonstrating using
 * the Footleg Graphics library to draw circles on a double buffered canvas
 * where pixels are double width or double height. Also demos drawing and
 * updating a vector polygon and drawing text onto the screen.
 *
 * Copyright (c) 2025 Dr Footleg
 *
 * License: GNU GPL v3.0
 */

#include "../libraries/graphics/footleg_graphics.hpp"
#include "drivers/st7701/st7701.hpp"
#include "hardware/dma.h"
#include "libraries/pico_graphics/pico_graphics.hpp"
#include "libraries/pico_vector/pico_vector.hpp"
#include "pico/multicore.h"
#include "pico/sync.h"

using namespace pimoroni;

// To get the best graphics quality without exceeding the RAM, use a width of
// 480 with a height of 240. All drawing commands will be done on a 480 x 480
// screen size, and scaled accordingly to draw round circles on the screen
// regardless of the buffer aspect ratio used.
#define FRAME_BUFFER_WIDTH 240
#define FRAME_BUFFER_HEIGHT 480

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

uint16_t screen_buffer[FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT];
uint16_t draw_buffer[FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT];

ST7701* presto;
PicoGraphics_PenRGB565* display;
FootlegGraphics* footlegGraphics;
PicoVector* vector;

int main() {
  set_sys_clock_khz(240000, true);
  stdio_init_all();

  gpio_init(LCD_CS);
  gpio_put(LCD_CS, 1);
  gpio_set_dir(LCD_CS, 1);

  presto = new ST7701(
      FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, ROTATE_0,
      SPIPins{spi1, LCD_CS, LCD_CLK, LCD_DAT, PIN_UNUSED, LCD_DC, BACKLIGHT},
      screen_buffer);
  display = new PicoGraphics_PenRGB565(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT,
                                       draw_buffer);
  footlegGraphics = new FootlegGraphics(display, draw_buffer);
  vector = new PicoVector(display);
  presto->init();

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

  float angle = 3.2f;
  int count = 0;
  float direction = 1.0f;

  pp_point_t outline[] = {{-36, -36}, {36, -36}, {36, 36}, {-36, 36}};
  pp_point_t hole[] = {{-16, 16}, {16, 16}, {16, -16}, {-16, -16}};

  pp_poly_t* poly = pp_poly_new();
  pp_path_add_points(pp_poly_add_path(poly), outline,
                     sizeof(outline) / sizeof(pp_point_t));
  pp_path_add_points(pp_poly_add_path(poly), hole,
                     sizeof(hole) / sizeof(pp_point_t));

  vector->translate(poly, {FRAME_BUFFER_WIDTH / 2.5, FRAME_BUFFER_HEIGHT / 2});

  while (true) {
    display->set_pen(BG);
    display->clear();

    footlegGraphics->drawCircle(30, 30, 1, RED);
    footlegGraphics->drawCircle(80, 30, 2, ORANGE);
    footlegGraphics->drawCircle(130, 30, 3, YELLOW);
    footlegGraphics->drawCircle(180, 30, 4, GREEN);
    footlegGraphics->drawCircle(230, 30, 5, BLUE);
    footlegGraphics->drawCircle(280, 30, 6, PURPLE);
    footlegGraphics->drawCircle(330, 30, 7, RED);
    footlegGraphics->drawCircle(380, 30, 8, ORANGE);
    footlegGraphics->drawCircle(430, 30, 9, YELLOW);

    footlegGraphics->drawCircleAA(30, 60, 1, RED);
    footlegGraphics->drawCircleAA(80, 60, 2, ORANGE);
    footlegGraphics->drawCircleAA(130, 60, 3, YELLOW);
    footlegGraphics->drawCircleAA(180, 60, 4, GREEN);
    footlegGraphics->drawCircleAA(230, 60, 5, BLUE);
    footlegGraphics->drawCircleAA(280, 60, 6, PURPLE);
    footlegGraphics->drawCircleAA(330, 60, 7, RED);
    footlegGraphics->drawCircleAA(380, 60, 8, ORANGE);
    footlegGraphics->drawCircleAA(430, 60, 9, YELLOW);

    footlegGraphics->drawCircle(30, 100, 10, RED);
    footlegGraphics->drawCircle(80, 100, 11, ORANGE);
    footlegGraphics->drawCircle(130, 100, 12, YELLOW);
    footlegGraphics->drawCircle(180, 100, 13, GREEN);
    footlegGraphics->drawCircle(230, 100, 14, BLUE);
    footlegGraphics->drawCircle(280, 100, 15, PURPLE);
    footlegGraphics->drawCircle(330, 100, 16, RED);
    footlegGraphics->drawCircle(380, 100, 17, ORANGE);
    footlegGraphics->drawCircle(430, 100, 18, YELLOW);

    footlegGraphics->drawCircleAA(30, 140, 10, RED);
    footlegGraphics->drawCircleAA(80, 140, 11, ORANGE);
    footlegGraphics->drawCircleAA(130, 140, 12, YELLOW);
    footlegGraphics->drawCircleAA(180, 140, 13, GREEN);
    footlegGraphics->drawCircleAA(230, 140, 14, BLUE);
    footlegGraphics->drawCircleAA(280, 140, 15, PURPLE);
    footlegGraphics->drawCircleAA(330, 140, 16, RED);
    footlegGraphics->drawCircleAA(380, 140, 17, ORANGE);
    footlegGraphics->drawCircleAA(430, 140, 18, YELLOW);

    display->set_pen(PINK);
    display->rectangle({20, display->bounds.h / 2, display->bounds.w - 40,
                        display->bounds.h - 110});
    display->set_pen(YELLOW);
    Point text_location = {30, display->bounds.h / 2 + 20};
    display->text("Hello Presto World!", text_location,
                  display->bounds.w - text_location.x);

    // Move the vector
    count++;
    if (count > 400) {
      direction = -direction;
      count = 0;
    }

    vector->rotate(poly, {FRAME_BUFFER_WIDTH / 2, FRAME_BUFFER_HEIGHT / 2},
                   angle);
    vector->translate(poly, {direction, direction});

    vector->draw(poly);

    presto->update(display);
    sleep_ms(40);
  }
}