# Bouncing Balls
Initially based on the drawing example code from Pimoroni, I added collisions between balls. Then things started to get out of hand as the ideas flooded in. 
I added the ability to add more balls using the buttons. Then I added 'force' mode where there is a repelling force between balls. The up and down buttons let 
you change the level of force. Make it negative and the balls are attracted to each other rather than repelled. Next came the inclusion of the size of the balls
in the amount of energy they impart in collisions or force they exert on each other. In particular the orbital mechanics when in force mode with negative force 
levels was fascinating, so I added the option to zoom out to give the balls more space. Infinite space! It does get a bit slower both due to the additional 
calculations required to draw each ball when zoomed out, but also because the distances the balls have to move increase, so they appear slower on the screen.

## Summary of Controls
Button A: Toggles between Bounce mode and Force mode.
Button B: Adds a ball for each press (or hold to add several)
Button C: Toggles mass on and off. When on (m) appears to indicate that the size of balls in taken into account.

Up/Down: These buttons increase/decrease the force level when mass is off, or zoom out/in when mass is on.

The zooming out is straight forward, as more space is just given to the balls. Zooming in snaps to just fit all the balls on the screen, so it only makes a
significant difference when all the balls are close together leaving space to crop from around them.

## Some fun settings
Add a 3rd ball (button B), switch to force mode (button A), then decrease the force to around -24 (down button). Now toggle on mass (button C), and finally zoom 
out a couple of levels (Up button, now that mass is applied). Watch the balls orbit each other in chaotic patterns! Add more balls at any time with button B.
You can reset to restart with just 2 balls by pressing the reset button on the back (twice).
