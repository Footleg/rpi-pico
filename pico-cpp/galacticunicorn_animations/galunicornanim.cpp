#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/platform.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#include "libraries/pico_graphics/pico_graphics.hpp"
#include "galactic_unicorn.hpp"

#include "crawler.h" //This is one of the animation classes used to generate output for the display
#include "golife.h"  //This is one of the animation classes used to generate output for the display
#include "gravityparticles.h"  //Another animation class used to generate output for the display

/*
  Example project for RGB matrix animations library.
*/

using namespace pimoroni;

PicoGraphics_PenRGB888 graphics(53, 11, nullptr);
GalacticUnicorn galactic_unicorn;

uint32_t time() {
  absolute_time_t t = get_absolute_time();
  return to_ms_since_boot(t);
}

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
        Animation(uint16_t steps_, uint16_t minSteps_, 
            uint8_t golFadeSteps_, uint16_t golDelay_, uint8_t golStartPattern_,
            uint16_t shake, uint8_t bounce_)
            : RGBMatrixRenderer{graphics.bounds.w,graphics.bounds.h}, 
              animCrawler(*this, steps_, minSteps_,false), 
              animGol(*this, golFadeSteps_, golDelay_, golStartPattern_),
              animParticles(*this,shake,bounce_)
        {
            //Clear screen
            Pen BG = graphics.create_pen(0, 0, 0);
            graphics.set_pen(BG);
            graphics.clear();

            //Precalculate pixel radius
            rad = 1;

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
                    break;
                case 2:
                    animParticles.runCycle();
                    if(cycles > 1000){
                        cycles = 0;
                        uint16_t ax = 0;
                        uint16_t ay = 0;
                        while(abs(ax) + abs(ay) < 200) {
                            ax = 200-rand()%400;
                            ay = 200-rand()%400;
                        }
                        animParticles.setAcceleration(ax,ay);
                    }
                    break;
            }
            cycles++;
        }

        virtual void showPixels() {
            galactic_unicorn.update(&graphics);
        }

        virtual void outputMessage(char msg[]) {
            if (msg[0] == 32){
                graphics.set_pen(WHITE);
                Point text_location(0, 0);
                graphics.text(msg, text_location, getGridWidth() );

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
            animParticles.setAcceleration(0,200);

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
        void setCrawlerAnyAngle(bool anyAngle) {
            animCrawler.anyAngle = anyAngle;
        }

    private:
        Crawler animCrawler;
        GameOfLife animGol;
        GravityParticles animParticles;
        uint8_t animationMode;
        Pen WHITE = graphics.create_pen(255, 255, 255);
        uint16_t cycles;

        virtual void setPixel(uint16_t x, uint16_t y, RGB_colour colour) 
        {
            graphics.set_pen(graphics.create_pen(colour.r, colour.g, colour.b));
            graphics.set_pixel(Point(x, y));
        }
};

int main() {

    stdio_init_all();

    galactic_unicorn.init();
    galactic_unicorn.set_brightness(0.5);

    uint16_t shake = 100;
    uint8_t bounce = 200;
    uint8_t pixelSize = 1;

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
    srand(int(seed*1000));

    uint8_t animationMode = 0;
    Animation animation(steps,minSteps,golFadeSteps,golDelay,golStartPattern,shake,bounce);

    uint8_t oldFadeSteps;
    uint8_t oldGolStartPattern;

    uint16_t loopDelay = 20;
    uint32_t loopstartTime = time();

    bool crawlerAnyAngle = false;

    while(true) {
        //Store config options so we can detect if any changed
        oldFadeSteps = golFadeSteps;
        oldGolStartPattern = golStartPattern;

        if(galactic_unicorn.is_pressed(galactic_unicorn.SWITCH_BRIGHTNESS_UP)) {
            galactic_unicorn.adjust_brightness(+0.01);
            sleep_ms(50);
        }
        if(galactic_unicorn.is_pressed(galactic_unicorn.SWITCH_BRIGHTNESS_DOWN)) {
            galactic_unicorn.adjust_brightness(-0.01);
            sleep_ms(50);
        }

        if(galactic_unicorn.is_pressed(galactic_unicorn.SWITCH_VOLUME_UP)) {
            if(galactic_unicorn.is_pressed(galactic_unicorn.SWITCH_D)) {
                if(animationMode == animModeGol){
                    if(golFadeSteps < 80) golFadeSteps++;
                }
            }
            else {
                loopDelay -= 10;
                if (loopDelay > 65000) loopDelay = 0;
            }
            sleep_ms(50);
        }
        if(galactic_unicorn.is_pressed(galactic_unicorn.SWITCH_VOLUME_DOWN)) {
            if(galactic_unicorn.is_pressed(galactic_unicorn.SWITCH_D)) {
                if(animationMode == animModeGol){
                    if(golFadeSteps > 1) golFadeSteps--;
                }
            }
            else {
                loopDelay += 10;
                if (loopDelay > 1000) loopDelay = 1000;
            }
            sleep_ms(50);
        }

        if(galactic_unicorn.is_pressed(galactic_unicorn.SWITCH_A)) {
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

            //Pause to prevent repeated button detections when in fast loop
            sleep_ms(500);
        }

        if(galactic_unicorn.is_pressed(galactic_unicorn.SWITCH_B)) {
            sleep_ms(500);
            if(animationMode == animModeGol){
                // golStartPattern++; <-- DISABLED as none of the preset patterns work well on the resolution of this matrix
                if(golStartPattern > 8) golStartPattern = 0;
            }
            else if(animationMode == animModeCrawler){
                crawlerAnyAngle = not crawlerAnyAngle;
                animation.setCrawlerAnyAngle(crawlerAnyAngle);
            }
            else if(animationMode == animModeParticles){
                // char msg[10];
                // sprintf(msg, "Footleg");
                // graphics.set_pen(graphics.create_pen(255, 100, 0));
                // Point text_location(0, 0);
                // graphics.text(msg, text_location, animation.getGridWidth() );
                // Draw some fixed pixel shapes
                RGB_colour yellow = {255,200,120};
                RGB_colour red = {255,0,0};
                RGB_colour blue = {0,0,255};

                for(uint16_t y = 3; y < 7; ++y)
                {
                    for(uint16_t x = 10; x < 14; ++x)
                    { 
                        //Clear cells outside pattern
                        animation.setPixelColour(x, y, blue);
                    }
                }

                animation.drawCircle(20,5,3,red);
                animation.updateDisplay();

           }
        } 

        if (time() - loopstartTime > loopDelay) {
            //Loop delay has elasped, so reset loop start time
            loopstartTime = time();

            //Update parameters if changed
            if (golFadeSteps != oldFadeSteps || golStartPattern != oldGolStartPattern){
                //Need to destroy and recreate animation objects
                animation.~Animation();
                new(&animation) Animation(steps,minSteps,golFadeSteps,golDelay,golStartPattern,shake,bounce);
            }

            //Increment animation cycle
            animation.animationStep();
        }

        sleep_ms(1);
    }

    return 0;
}