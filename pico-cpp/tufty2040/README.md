# Applications for the [Tufty2040 Badge](https://shop.pimoroni.com/products/tufty-2040)

## Tufty Balls

Initially based on the drawing example code from Pimoroni, I added collisions between balls. Then things started to get out of hand as the ideas flooded in.
I added the ability to add more balls using the buttons. Then I added 'force' mode where there is a repelling force between balls. The up and down buttons let
you change the level of force. Make it negative and the balls are attracted to each other rather than repelled. Next came the inclusion of the size of the balls
in the amount of energy they impart in collisions or force they exert on each other. In particular the orbital mechanics when in force mode with negative force
levels was fascinating, so I added the option to zoom out to give the balls more space. Infinite space! It does get a bit slower both due to the additional
calculations required to draw each ball when zoomed out, but also because the distances the balls have to move increase, so they appear slower on the screen.

### Summary of Controls

- Button A: Toggles between Bounce mode and Force mode.
- Button B: Adds a ball for each press (or hold to add several)
- Button C: Toggles mass on and off. When on (m) appears to indicate that the size of balls in taken into account.

- Up/Down: These buttons increase/decrease the force level when mass is off, or zoom out/in when mass is on.

The zooming out is straight forward, as more space is just given to the balls. Zooming in snaps to just fit all the balls on the screen, so it only makes a
significant difference when all the balls are close together leaving space to crop from around them.

### Some fun settings

Add a 3rd ball (button B), switch to force mode (button A), then decrease the force to around -24 (down button). Now toggle on mass (button C), and finally zoom
out a couple of levels (Up button, now that mass is applied). Watch the balls orbit each other in chaotic patterns! Add more balls at any time with button B.
You can reset to restart with just 2 balls by pressing the reset button on the back (twice).

## RGB Matrix Animations

This example shows how you can run my hardware independent RGB LED animations library on a display. Originally written for LED matrix devices, I realised it was useful to play with the same code on devices with screens. Ultimately a screen is a matrix of RGB pixels of course, but running these animation classes at that resolution of pixels is rather slow. So instead we can draw circles on the screen to represent LEDs on a matrix. A sort of LED matrix simulation, allowing the code to be played with on a screen.

There are several animations in the library.

- Conway's Game of Life
- Crawler (a test for movement commands across multi-panel LED matrix installations)
- Gravity Particles (LEDs act like particles which fall and collide under forces applied to the matrix space)

### Animations Controls

- Up and Down: Change the simulated pixel size on the screen, from 20 pixel diameter circles, all the way down to 2 pixel diameter. We can't do 1 pixel LEDs on the Tufty2040 as it crashes the cpu.
This option restarts the animation at the new resolution, so always resets to a fresh Game of Life.
- Button A: Switches between the animations. Game of Life -> Crawler -> Gravity particles (turns the screen contents to falling sand).
- Button B: Toggles different behaviours in each animation mode.
- Button C: If held down while pressing the Up or Down buttons, enables up/down to change the number of fade steps in Game of Life (where cells grow by fading in green, and die by fading to red).
