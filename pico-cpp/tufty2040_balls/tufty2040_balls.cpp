#include "pico/stdlib.h"
#include <stdio.h>
#include <cstring>
#include <string>
#include <algorithm>
#include "pico/time.h"
#include "pico/platform.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#include "common/pimoroni_common.hpp"
#include "drivers/st7789/st7789.hpp"
#include "libraries/pico_graphics/pico_graphics.hpp"
#include "tufty2040.hpp"
#include "button.hpp"

using namespace pimoroni;

Tufty2040 tufty;

ST7789 st7789(
  Tufty2040::WIDTH,
  Tufty2040::HEIGHT,
  ROTATE_0,
  ParallelPins{
    Tufty2040::LCD_CS,
    Tufty2040::LCD_DC,
    Tufty2040::LCD_WR,
    Tufty2040::LCD_RD,
    Tufty2040::LCD_D0, 
    Tufty2040::BACKLIGHT
  }
);

PicoGraphics_PenRGB332 graphics(st7789.width, st7789.height, nullptr);

Button button_a(Tufty2040::A,Polarity::ACTIVE_HIGH);
Button button_b(Tufty2040::B,Polarity::ACTIVE_HIGH);
Button button_c(Tufty2040::C,Polarity::ACTIVE_HIGH);
Button button_up(Tufty2040::UP,Polarity::ACTIVE_HIGH);
Button button_down(Tufty2040::DOWN,Polarity::ACTIVE_HIGH);

uint32_t time() {
  absolute_time_t t = get_absolute_time();
  return to_ms_since_boot(t);
}

// HSV Conversion expects float inputs in the range of 0.00-1.00 for each channel
// Outputs are rgb in the range 0-255 for each channel
void from_hsv(float h, float s, float v, uint8_t &r, uint8_t &g, uint8_t &b) {
  float i = floor(h * 6.0f);
  float f = h * 6.0f - i;
  v *= 255.0f;
  uint8_t p = v * (1.0f - s);
  uint8_t q = v * (1.0f - f * s);
  uint8_t t = v * (1.0f - (1.0f - f) * s);

  switch (int(i) % 6) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    case 5: r = v; g = p; b = q; break;
  }
}

struct pt {
  float     x;
  float     y;
  uint8_t    r;
  float     dx;
  float     dy;
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
  // Initialise random numbers seed using floating adc input reading
  adc_init();
  // Make sure GPIO is high-impedance, no pullups etc
  adc_gpio_init(28);
  // Select ADC input 2 (GPIO28) which is available on the Tufty2040 board
  adc_select_input(2);
  float seed = 0.0f;
  for(int it = 0; it < 100; it++){
    seed += adc_read();
  }
  srand(int(seed));

  st7789.set_backlight(255);

  Pen WHITE = graphics.create_pen(255, 255, 255);
  Pen BG = graphics.create_pen(0, 0, 0);

  std::vector<pt> shapes;

  for(int i = 0; i < 2; i++) {
    pt newShape = createShape();
    shapes.push_back(newShape);
  }

  Point text_location(0, 0);
  uint8_t ledbri = 0;

  //Simulation boundaries
  float minX = 0;
  float minY = 0;
  float maxX = graphics.bounds.w;
  float maxY = graphics.bounds.h;
  uint8_t renderSkip = 1;
  uint8_t renderCount = 0;

  uint16_t i = 0;
  uint8_t mode = 0;
  float forcePower = 2.0f;
  float step = 2.0f;
  bool mass = false;

  while(true) {
    if(renderCount == 0){
      graphics.set_pen(BG);
      graphics.clear();
    }

    i = 0;
    for(auto &shape : shapes) {
      //Update shape position
      shape.x += shape.dx;
      shape.y += shape.dy;

      //Check for collision with shapes already updated
      for(uint8_t j = 0; j < i; j++) {
        //Check distance between shapes
        float sepx = shapes[j].x - shape.x;
        float sepy = shapes[j].y - shape.y;
        uint16_t sep = int(sqrt((sepx*sepx)+(sepy*sepy)));

        //Don't try to process interactions if shapes exactly on top of one another
        if(sep > 0.0) {

          float ax = 0.0f;
          float ay = 0.0f;

          uint8_t rd = 0;
          float force = forcePower;

          //Bounce if contacting
          rd = shape.r + shapes[j].r;
          if(sep < rd) {
            /*
            //Trig solution
            float angle = atan2(sepy, sepx);
            float targetX = shape.x + cos(angle) * sep;
            float targetY = shape.y + sin(angle) * sep;
            float ax = (targetX - shape.x);
            float ay = (targetY - shape.y);
            */

            //Handle x and y components seperately (more efficient)
            ax = sepx;
            ay = sepy;
          }
          else {
            switch(mode){
            case 1:
              //Repel, Force is inverse of distance squared
              force = force / (sep*sep);
              ax = force * sepx / sep;
              ay = force * sepy / sep;

              break;
            }
          }

          float prePower = sqrt(shape.dx*shape.dx+shape.dy*shape.dy) + sqrt(shapes[j].dx*shapes[j].dx+shapes[j].dy*shapes[j].dy);
          if(mass){
            shape.dx -= ax * shapes[j].r;
            shape.dy -= ay * shapes[j].r;
            shapes[j].dx += ax * shape.r;
            shapes[j].dy += ay * shape.r;
          }
          else {
            shape.dx -= ax * 10;
            shape.dy -= ay * 10;
            shapes[j].dx += ax * 10;
            shapes[j].dy += ay * 10;
          }

          float postPower = sqrt(shape.dx*shape.dx+shape.dy*shape.dy) + sqrt(shapes[j].dx*shapes[j].dx+shapes[j].dy*shapes[j].dy);
          float scalePower = prePower / postPower;

          shape.dx = shape.dx * scalePower;
          shape.dy = shape.dy * scalePower;
          shapes[j].dx = shapes[j].dx * scalePower;
          shapes[j].dy = shapes[j].dy * scalePower;

          
        }
      }

      //Check shape remains in bounds of screen, reverse direction if not
      if((shape.x - shape.r) < minX) {
        shape.dx *= -1;
        shape.x = minX + shape.r;
      }
      if((shape.x + shape.r) >= maxX) {
        shape.dx *= -1;
        shape.x = maxX - shape.r;
      }
      if((shape.y - shape.r) < minY) {
        shape.dy *= -1;
        shape.y = minY + shape.r;
      }
      if((shape.y + shape.r) >= maxY) {
        shape.dy *= -1;
        shape.y = maxY - shape.r;
      }

      if(renderCount == 0){
        //Skip the slow calcs if 1:1 scale with screen
        if(minX == 0 && minY == 0){
          //Draw circles at 1:1 scale on screen
          graphics.set_pen(shape.pen);
          graphics.circle(Point(shape.x, shape.y), shape.r);
        }
        else {
          //Draw circles scaled to boundaries
          float posX = graphics.bounds.w * (shape.x - minX) / (maxX - minX);
          float posY = graphics.bounds.h * (shape.y - minY) / (maxY - minY);
          float rad = graphics.bounds.h * shape.r / (maxY - minY);
          if(rad < 2) rad = 2;
          graphics.set_pen(shape.pen);
          graphics.circle(Point(posX, posY), rad);

        }
      }

      i++;
    }

    renderCount++;
    if(renderCount > renderSkip) renderCount = 0;

    char msg[20];
    if(mass) {
      switch(mode) {
        case 0:
          sprintf(msg, "Bounce (m)");
          break;
        case 1:
          sprintf(msg, "Force %.1f (m)", forcePower);
          break;
        default:
          sprintf(msg, "Who knows %.1f (m)", forcePower);
      }
    }
    else {
      switch(mode) {
        case 0:
          sprintf(msg, "Bounce");
          break;
        case 1:
          sprintf(msg, "Force %.1f", forcePower);
          break;
        default:
          sprintf(msg, "Who knows %.1f", forcePower);
      }
    }
    graphics.set_pen(WHITE);
    graphics.text(msg, text_location, 320);

    // update screen
    st7789.update(&graphics);

    if(button_a.read()) {
      mode++;
      if(mode > 1) mode = 0;
    }

    if(button_b.read()) {
        pt newShape = createShape();
        shapes.push_back(newShape);
    }
    if(button_c.read()) {
        mass = !mass;
    }
    if(button_up.read()) {
      if(mass){
        //Find current bounds and multiply by 1.5
        float minXc = shapes[0].x;
        float minYc = shapes[0].y;
        float maxXc = minXc;
        float maxYc = minYc;
        for(auto &shape : shapes) {
          if(shape.x < minXc) minXc = shape.x;
          if(shape.x > maxXc) maxXc = shape.x;
          if(shape.y < minYc) minYc = shape.y;
          if(shape.y > maxYc) maxYc = shape.y;
        }
        float midX = minXc + (maxXc - minXc)/2;
        float midY = minYc + (maxYc - minYc)/2;
        float addX = (maxX - minX) * 1.5 / 2;
        float addY = (maxY - minY) * 1.5 / 2;
        minX -= addX;
        maxX += addX;
        minY -= addY;
        maxY += addY;

        renderSkip++;
      }
      else {
        forcePower += step;
      }
    }
    if(button_down.read()) {
      if(mass){
        //Find current bounds shink area to just include them
        float minXc = shapes[0].x;
        float minYc = shapes[0].y;
        float maxXc = minXc;
        float maxYc = minYc;
        for(auto &shape : shapes) {
          if(shape.x < minXc) minXc = shape.x;
          if(shape.x > maxXc) maxXc = shape.x;
          if(shape.y < minYc) minYc = shape.y;
          if(shape.y > maxYc) maxYc = shape.y;
        }
        float midX = minXc + (maxXc - minXc)/2;
        float midY = minYc + (maxYc - minYc)/2;
        float totX = maxXc - minXc;
        float totY = maxYc - minYc;
        if(totX > totY * graphics.bounds.w / graphics.bounds.h){
          totY = totX * graphics.bounds.h / graphics.bounds.w;
        }
        else {
          totX = totY * graphics.bounds.w / graphics.bounds.h;
        }
        minX = midX - totX/1.95;
        maxX = midX + totX/1.95;
        minY = midY - totY/1.95;
        maxY = midY + totY/1.95;

        if(renderSkip > 0) renderSkip--;

        if(maxX - minX < graphics.bounds.w){
          //Reset to 1:1 scale
          minX = 0;
          minY = 0;
          maxX = graphics.bounds.w;
          maxY = graphics.bounds.h;
          renderSkip = 1;
        }
      }
      else {
        forcePower -= step;
      }
    }
    if( abs(forcePower) < 4 ){
      step = 0.1;
    }
    else if( abs(forcePower) < 20 ){
      step = 1.0;
    }
    else if( abs(forcePower) < 40 ){
      step = 2.0;
    }
    else if( abs(forcePower) < 80 ){
      step = 4.0;
    }
    else {
      step = 10.0;
    }

    //sleep_ms(50);
  }

    return 0;
}
