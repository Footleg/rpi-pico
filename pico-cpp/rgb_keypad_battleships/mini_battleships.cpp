#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#include "pico_rgb_keypad.hpp"

/*
  Press buttons to find the ships in the sea.
*/

using namespace pimoroni;

PicoRGBKeypad pico_keypad;

// Simple struct to pair r/g/b together as a color
struct color {uint8_t r, g, b;};

//Convert x and y coordinates into button index
int btnFromXY(int x, int y) {
    int btnIdx = -1;
    if(x >= 0 && x < 4 && y >= 0 && y < 4) {
        btnIdx = y * 4 + x;
    }
    return btnIdx;
}

//Get allowed random direction of movement from x,y position
int rndDirFromPos(int x, int y) {
    int direction = 0;

    //Initialise direction to random 0 or 1
    while ((direction = rand() & 0x0f) > 1) {}
    if(x==0){
        if(y==0){
            //Set direction to either east or south (1 or 2)
            direction++;
        }
        else if(y==3){
            //Set direction to either north or east (0 or 1)
        }
        else {
            direction = 1; //East
        }
    }
    else if(y==0){
        if(x==3){
            //Set direction to either south or west (2 or 3)
            direction += 2;
        }
        else {
            direction = 2; //South
        }
    }
    else if(y==3){
        if(x==3){
            //Set direction to either north or west (0 or 3)
            if(direction > 0) {
                direction = 3;
            }
        }
        else {
            direction = 0; //North
        }
    }
    else if(x==3){
        direction = 3; //West
    }
    else {
        //Any direction allowed
        while ((direction = rand() & 0x0f) > 3) {}
    }

    return direction;
}

uint16_t setShipLocations() {
    uint16_t locs = 0;
    int x = 0;
    int y = 0;
    bool edge = false;

    //Place 3 long ship, starting from an edge position
    while(edge == false) {
        while ((x = rand() & 0x0f) > 3) {}
        while ((y = rand() & 0x0f) > 3) {}
        edge = (x == 0 || x == 3 || y == 0 || y == 3);
    }

    //Get random direction to extend from this position
    int direction = rndDirFromPos(x,y);

    for(int i=0; i<3; i++) {
        int pos = btnFromXY(x,y);
        locs |= (0b1 << pos);
        switch(direction) {
        case 0:
            // Move North
            y += -1;
            break;
        case 1:
            // Move East
            x++;
            break;
        case 2:
            // Move South
            y++;
            break;
        default:
            // Move West
            x += -1;
        }
    }

    //Place 2 long ship by finding 2 random adjacent positions which are free
    bool clear = false;
    while(clear == false) {
        while ((x = rand() & 0x0f) > 3) {}
        while ((y = rand() & 0x0f) > 3) {}
        int pos1 = btnFromXY(x,y);
        direction = rndDirFromPos(x,y);
        switch(direction) {
        case 0:
            // Move North
            y += -1;
            break;
        case 1:
            // Move East
            x++;
            break;
        case 2:
            // Move South
            y++;
            break;
        default:
            // Move West
            x += -1;
        }
        int pos2 = btnFromXY(x,y);
        clear = not( (locs & (0b1 << pos1)) || (locs & (0b1 << pos2)) );
        if( clear ) {
            locs |= (0b1 << pos1);
            locs |= (0b1 << pos2);
        }
    }

    return locs;
}

int main() {
    adc_init();
    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(26);
    // Select ADC input 0 (GPIO26)
    adc_select_input(0);
    srand(adc_read());

    pico_keypad.init();
    pico_keypad.set_brightness(1.0f);

    uint16_t ship_locations = setShipLocations();

    uint16_t lit_buttons = 0;
    uint16_t prev_btn_states = pico_keypad.get_button_states();
    int hits = 0;
    int attempts = 0;

    while(true) {
        // Read button states from i2c expander
        // for any pressed buttons set the corresponding bit in "lit_buttons"
        lit_buttons |= pico_keypad.get_button_states();

        // Iterate through the lights
        hits = 0;
        attempts = 0;
        for(auto i = 0u; i < PicoRGBKeypad::NUM_PADS; i++) {
            if(lit_buttons & (0b1 << i)) {
                if(ship_locations & (0b1 << i)) {
                    // Hit = red
                    pico_keypad.illuminate(i, 0x80, 0x00, 0x00);
                    hits++;
                } else {
                    // Miss = sea
                    pico_keypad.illuminate(i, 0x00, 0x00, 0x80);
                }
                attempts++;
            } else {
                // Kinda dim white-ish
                pico_keypad.illuminate(i, 0x05, 0x05, 0x05);
            }
        }

        // Display the LED changes
        pico_keypad.update();

        
        if( prev_btn_states != pico_keypad.get_button_states() ) {
            //Btn state has changed, so wait until it changes again before changing lights again
            prev_btn_states = pico_keypad.get_button_states();
            //Wait until btn state changes
            while( prev_btn_states == pico_keypad.get_button_states() ) {
                sleep_ms(10);
            }
        }
        
        if ( hits > 4) {
            sleep_ms(2000); // Wait a little so the final state is shown
            lit_buttons = 0; // Reset lit buttons
            ship_locations = setShipLocations();
            //Show score
            for(auto i = 0u; i < PicoRGBKeypad::NUM_PADS; i++) {
                if(i < attempts) {
                    pico_keypad.illuminate(i, 0x00, 0x80, 0x00);
                } else {
                    // Kinda dim white-ish
                    pico_keypad.illuminate(i, 0x00, 0x00, 0x80);
                }
            }
            pico_keypad.update();
            sleep_ms(2000); // Wait a little so the score is shown
        }
    }

    return 0;
}
