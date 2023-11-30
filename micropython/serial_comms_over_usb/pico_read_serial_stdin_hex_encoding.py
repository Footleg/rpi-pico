"""
Example micropython program to run a Pico board which will be controlled
by data send over serial via USB from a host computer

Copyright (c) 2023 Dr Footleg

License: MIT

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

"""
from utime import ticks_ms, sleep
import sys
import select
import machine
import micropython

"""
A program to show that communication to the Pico is active and responding to serial data
The active loop is indicated by flashing the onboard LED rapidly.
The serial commands received are used to control the attached neopixels
"""

# Disable Ctrl+C interupt. Uncomment these lines to prevent the program being stopped
# by Ctrl+C. Note this will also prevent you stopping the program from your editor.
# The sleep statement is to give you time to stop the script on power up if you set
# this program to run automatically as main.py
#time.sleep(5)  #Sleep to allow time to stop program on power up to get to repl
#micropython.kbd_intr(-1)

# Constants
led = machine.Pin(25, machine.Pin.OUT)

# Global variables
last_data_time = ticks_ms()
last_data_msg = False
ledToggle = False                   # State flag for toggling on board LEDs
last_time = ticks_ms()              # Time of last LED toggle
state = 0                           # State machine state, for tracking serial commands
divider = 1                         # Used in test for exception handling

try:

    # Loop forever, or until a stop command from Thonny or another IDE is received
    while True:
        # Check for serial data with minimal timeout
        charRead = select.select([sys.stdin], [], [], 0.01)
        if charRead[0]:
            # Data received
            last_data_time = ticks_ms()
            if last_data_msg:
                print("Data resumed")
            last_data_msg = False

            ch = sys.stdin.read(1)
            print(f"Read:{ch} Binary:{ord(ch)}")
            
            if state == 0:
                # First nibble as an ascii char
                start_time = ticks_ms()
                # Reset the value
                btyeValue = 0
                # Check for supported command codes
                if (ch == 'l'):
                    # Code to keep LED on for 0.2 seconds
                    led.value(True)
                    sleep(0.2)
                elif (ch == 'x'):
                    # Cause an exception in main loop to test recovery from crashes
                    divider = 0
                else:
                    charVal = ord(ch)
                    if 47 < charVal < 58 or 96 < charVal < 67:
                        # Read high nibble of a byte being send as a pair of chars
                        ch1 = ch
                        # Increment state so we treat next char as the low nibble
                        state = 1
                        
            else:
                # Read low nibble of a byte being send as a pair of chars
                btyeValue = bytes.fromhex(ch1 + ch)[0]
                
                print(f"Value:{hex(btyeValue)}")
                state = 0
                
            if state == 0:
                print(f"Time taken: {ticks_ms() - start_time}")
                
            # Triggers an exception if 'x' was sent over serial
            divcrasher = 45 / divider
                
           
        # Toggle on board LED to show code is alive
        if ticks_ms() - last_time > 50:
            last_time = ticks_ms()
            led.value(ledToggle)
            ledToggle = not ledToggle
            
        if last_data_msg == False and ticks_ms() - last_data_time > 5000:
            # Report in console when no data has been read in over 5 seconds
            print("No data for 5 seconds")
            last_data_msg = True # Set flag to prevent repeating no data message
            
finally:
    # Unplanned exception is the only way we can exit the main loop
    print("exception")
    
    # Put the any hardware back into a safe state, regardless of how the program may have ended
    
    # Reboot the Pico so program restarts after an error
    machine.reset()
