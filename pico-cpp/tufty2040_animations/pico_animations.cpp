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

#include "crawler.h" //This is one of the animation classes used to generate output for the display
#include "golife.h"  //This is one of the animation classes used to generate output for the display
#include "gravityparticles.h"  //Another animation class used to generate output for the display

/*
  Example project for RGB matrix animations library.
*/

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

const uint8_t animModeGol = 0;
const uint8_t animModeCrawler = 1;
const uint8_t animModeParticles = 2;


uint16_t steps = 10;
uint16_t minSteps = 2;

uint8_t golFadeSteps = 1;
uint16_t golDelay = 1;
uint8_t golStartPattern = 0;

// RGB Matrix class which passes itself as a renderer implementation into the animation class.
// Needs to be declared before the global variable accesses it in this file.
// Passed as a reference into the animation class, so need to dereference 'this' which is a pointer
// using the syntax *this
class Animation : public RGBMatrixRenderer {
    public:
        Animation(uint8_t pixScale, 
            uint16_t steps_, uint16_t minSteps_, 
            uint8_t golFadeSteps_, uint16_t golDelay_, uint8_t golStartPattern_,
            uint16_t shake, uint8_t bounce_)
            : RGBMatrixRenderer{graphics.bounds.w/pixScale,graphics.bounds.h/pixScale}, 
              animCrawler(*this, steps_, minSteps_,true), 
              animGol(*this, golFadeSteps_, golDelay_, golStartPattern_),
              animParticles(*this,shake,bounce_),
              pixelSize(pixScale)
        {
            //Clear screen
            Pen BG = graphics.create_pen(0, 0, 0);
            graphics.set_pen(BG);
            graphics.clear();

           //Precalculate pixel radius
            if(pixelSize > 7){
                rad = pixelSize/2-1;
            }
            else if(pixelSize > 6){
                rad = 3;
            }
            else if(pixelSize > 1){
                rad = 2;
            }
            else{
                rad = 1;
            }

            //Initialise mode
            animationMode = 0;
            cycles = 0;
        }

        virtual ~Animation(){
            // animGol.~GameOfLife();
            // animCrawler.~Crawler();
            // animParticles.~GravityParticles();
        }

        void animationStep() {
            switch (animationMode) {
                case 0:
                    animGol.runCycle();
                   break;
                case 1:
                    animCrawler.runCycle();
                    sleep_ms(1);
                    break;
                case 2:
                    animParticles.runCycle();
                    if(cycles > 1000){
                        cycles = 0;
                        animParticles.setAcceleration(100-rand()%200,100-rand()%200);
                    }
                    sleep_ms(1);
                    break;
            }
            cycles++;
        }

        virtual void showPixels() {
            st7789.update(&graphics);
        }

        virtual void outputMessage(char msg[]) {
            if (msg[0] == 32){
                graphics.set_pen(WHITE);
                Point text_location(0, 0);
                graphics.text(msg, text_location, 320);

            }
        }
        
        virtual void msSleep(int delay_ms) {
            sleep_ms(delay_ms);
        }

        virtual int16_t random_int16(int16_t a, int16_t b) {
            return a + rand()%(b-a);
        }

        void setMode(uint8_t mode) {
            cycles = 0;
            animationMode = mode;
        }

        void setParticles() {
            //Set particles
            animParticles.setAcceleration(0,-50);

            animParticles.clearParticles();
            animParticles.imgToParticles();

            // RGB_colour yellow = {255,200,120};

            // //Add grains in random positions with random initial velocities
            // int16_t maxVel = 10000;
            // for (int i=0; i<numParticles; i++) {
            //     //animation.addParticle( getRandomColour() );
            //     int16_t vx = random_int16(-maxVel,maxVel+1);
            //     int16_t vy = random_int16(-maxVel,maxVel+1);
            //     if (vx > 0) {
            //         vx += maxVel/5;  
            //     }
            //     else {
            //         vx -= maxVel/5;
            //     }
            //     if (vy > 0) {
            //         vy += maxVel/5;  
            //     }
            //     else {
            //         vy -= maxVel/5;
            //     }
            //     animParticles.addParticle( yellow, vx, vy );
            // }
        }

        uint16_t particleCount(){
            return animParticles.getParticleCount();
        }

        void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2 ){
            //Get smaller and larger points
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

            RGB_colour yellow = {255,200,120};
            RGB_colour red = {255,0,0};
            RGB_colour blue = {0,0,255};

            if (xb-xa == 0){
                if (y2 > y1) {
                    ya = y1;
                    yb = y2;
                }
                else {
                    ya = y2;
                    yb = y1;
                }
                for(uint16_t y=ya; y<=yb; y++){
                    setPixelColour(xa,y,yellow);
                }
            }
            else {
                for(uint16_t x=xa; x<=xb; x++){
                    uint16_t y = ya + (yb-ya) * (x-xa)/(xb-xa);
                    setPixelColour(x,y,yellow);
                }
                //Mark ends
                setPixelColour(x1,y1,red);
                setPixelColour(x2,y2,blue);
            }

            updateDisplay();
        }

        uint8_t rad;

    private:
        Crawler animCrawler;
        GameOfLife animGol;
        GravityParticles animParticles;
        uint8_t animationMode;
        Pen WHITE = graphics.create_pen(255, 255, 255);
        uint16_t cycles;
        uint8_t pixelSize;

        virtual void setPixel(uint16_t x, uint16_t y, RGB_colour colour) 
        {
            graphics.set_pen(graphics.create_pen(colour.r, colour.g, colour.b));
            if(pixelSize == 1){
                graphics.set_pixel(Point(x+50, y+45));
            }
            else if(pixelSize < 4){
                graphics.set_pixel(Point(x*pixelSize+1, y*pixelSize+1));
            }
            else if(pixelSize > 5){
                graphics.circle(Point(x*pixelSize+rad+1, y*pixelSize+rad+1), rad);
            }
            else {
                //Pixel sizes 4 or 5 draw four pixel block as circle does not draw nicely at this size
                graphics.set_pixel(Point(x*pixelSize+1, y*pixelSize+1));
                graphics.set_pixel(Point(x*pixelSize+1, y*pixelSize+2));
                graphics.set_pixel(Point(x*pixelSize+2, y*pixelSize+1));
                graphics.set_pixel(Point(x*pixelSize+2, y*pixelSize+2));
            }
            
        }
};


int main() {
    uint16_t shake = 100;
    uint8_t bounce = 200;
    uint8_t pixelSize = 20;

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

    uint8_t animationMode = 0;
    Animation animation(pixelSize,steps,minSteps,golFadeSteps,golDelay,golStartPattern,shake,bounce);

    uint8_t oldPixelSize;
    uint8_t oldFadeSteps;
    uint8_t oldGolStartPattern;

    while(true) {
        //Call animation class method to run a cycle
        animation.animationStep();

        //Store config options so we can detect if any changed
        oldPixelSize = pixelSize;
        oldFadeSteps = golFadeSteps;
        oldGolStartPattern = golStartPattern;

        if(button_up.read()) {
            if(button_c.read()) {
                if(animationMode == animModeGol){
                    if(golFadeSteps < 80) golFadeSteps++;
                }
            }
            else {
                if(pixelSize < 20) pixelSize++;
            }
        }

        if(button_down.read()) {
            if(button_c.read()) {
                if(animationMode == animModeGol){
                    if(golFadeSteps > 1) golFadeSteps--;
                }
            }
            else {
                if(pixelSize > 1) pixelSize--;
            }
            
        }

        if(button_a.read()) {
            animationMode++;
            if(animationMode > 2) animationMode = 0;
            animation.setMode(animationMode);

            //Clear all pixels or convert to particles
            if(animationMode == animModeParticles){
                animation.setParticles();
            }
            else {
                animation.clearImage();
            }
            animation.updateDisplay();
        }

        if(button_b.read()) {
            if(animationMode == animModeGol){
                golStartPattern++;
                if(golStartPattern > 7) golStartPattern = 0;
            }
            else if(animationMode == animModeParticles){
                //animation.drawLine(animation.random_int16(20,100),animation.random_int16(20,80),animation.random_int16(20,100),animation.random_int16(20,80));
                uint16_t xl = animation.getGridWidth() * 0.8;
                uint16_t yl = animation.getGridHeight() * 0.7;
                animation.drawLine(animation.getGridWidth()/2,yl,xl,animation.getGridHeight()/2);
                animation.drawLine(animation.getGridWidth()/2,animation.getGridHeight()-yl,xl,animation.getGridHeight()/2);
                animation.drawLine(animation.getGridWidth()/2,yl,animation.getGridWidth()-xl,animation.getGridHeight()/2);
                animation.drawLine(animation.getGridWidth()/2,animation.getGridHeight()-yl,animation.getGridWidth()-xl,animation.getGridHeight()/2);
            }
        }

        if (pixelSize != oldPixelSize || golFadeSteps != oldFadeSteps || golStartPattern != oldGolStartPattern){
            //Need to destroy and recreate animation objects
            animation.~Animation();
            if(golStartPattern > 0 && golStartPattern != 3){
                pixelSize = 4;
                new(&animation) Animation(pixelSize,steps,minSteps,golFadeSteps,golDelay,golStartPattern,shake,bounce);
            }
            else if(pixelSize > 1){
                new(&animation) Animation(pixelSize,
                    steps,minSteps,golFadeSteps,golDelay,golStartPattern,shake,bounce);
            }
            else {
                new(&animation) Animation(2,steps,minSteps,golFadeSteps,golDelay,golStartPattern,shake,bounce);
            }
        }
    }

    return 0;
}