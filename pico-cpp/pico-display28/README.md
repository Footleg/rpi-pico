# Applications for the Pico2 using the [Pimoroni Display Pack 2.8"](https://shop.pimoroni.com/products/pico-display-pack-2-8)

This project is set up for development using the Raspberry Pi Pico Extension in VSCode. To build this project, install Raspberry Pi Pico Extension in VSCode and then open the pico-display28 folder in VSCode. Be sure you cloned the git repo containing these projects using the --recursive flag, to fetch the required dependencies. The project should then build successfully when you click the compile button on the task bar at the bottom of the VSCode window which the Raspberry Pi Pico Extension provides. Make sure you set the board to match the pico variant you have attached to the display.

Applications in this project:

## Buttons Demo

Demonstration of the enhanced button class I wrote to support short press, long press and changes in response the longer a button is held. Allows improved user interfaces with multiple functions per button depending how it is pressed (quick press does something different to a long press).

## Balls Simulation

Initially based on the drawing example code from Pimoroni, I added collisions between balls. Then things started to get out of hand as the ideas flooded in. I added the ability to add more balls using the buttons. Then I added 'force' mode where there is a repelling force between balls. The up and down buttons let you change the level of force. Make it negative and the balls are attracted to each other rather than repelled. Next came the inclusion of the size of the balls in the amount of energy they impart in collisions or force they exert on each other. In particular the orbital mechanics when in force mode with negative force levels was fascinating, so I added the option to zoom out to give the balls more space. Infinite space! It does get a bit slower both due to the additional calculations required to draw each ball when zoomed out, but also because the distances the balls have to move increase, so they appear slower on the screen. This started as a demo on the tufty2040 badge, but runs really nicely on the newer Pico2 with the floating point unit and faster CPU. I have added a frames per second calculation and it manages over 33 fps with 24 balls in the simulation even when zoomed out (which runs slower due to the scaling calculation overhead).

### Summary of Controls

I wrote the enhanced button class for this, which supports short and long presses, and allows changes in behaviour the longer a button is held down.

On a short press (press and release within 0.6 seconds):

- Button A: Toggles between Bounce mode and Force mode.
- Button B: Adds a ball for each press
- Button X: Zooms in.
- Button Y: Zooms out.

The zooming out is straight forward, as more space is just given to the balls. Zooming in snaps to just fit all the balls on the screen, so it only makes a significant difference when all the balls are close together leaving space to crop from around them.

On holding down the button:

- Button A: Moves the text up the screen.
- Button B: Moves the text down the screen (you can hide it off the bottom of the screen).
- Button X: Increases the force between the balls.
- Button Y: Decreases the force between the balls.

The longer a button is held, the faster the rate of change of it's setting.

Positive force levels mean the balls repel each other. Negative force levels mean the balls are attracted to each other.

### Some fun settings

Add a 3rd ball (button B), switch to force mode (button A), then decrease the force to around -24 (down button). Now zoom out a couple of levels. Watch the balls orbit each other in chaotic patterns! Add more balls at any time with button B. You can reset to restart with just 2 balls by pressing the reset button on the Pico2 board (if using a Pimoroni version which has a reset button). Days of your life go by playing with this thing.
