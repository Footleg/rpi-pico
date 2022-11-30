"""
Galactic Unicorn Cheerlights MQTT listener.

Copyright (c) 2022 Dr Footleg

A simple demonstration of MQTT using the connect_mqtt module.
This program connects to a wifi network and then listens for messages on
the cheerlights MQTT server. For each change in the cheerlights colour,
it updates the LEDs on a Neopixel strip, showing the history of colours
received over time. It will automatically attempt to reconnect to the same
MQTT server if any (handled) network exceptions occur.

REQUIRED HARDWARE:
* Pimoroni Galactic Unicorn board with PicoW aboard

License: MIT

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

"""
import time
import random
import machine
import struct
import connect_mqtt
from machine import Timer, Pin
from galactic import GalacticUnicorn
from picographics import PicoGraphics, DISPLAY_GALACTIC_UNICORN as DISPLAY

MQTTCHECK_INTERVAL = 20  # Interval in seconds between checks for MQTT messages

mqtt_topic = 'cheerlightsRGB'

# Counter for the number of messages received
msgCounter = 0
checkCount = 0

# Track last colour seen to avoid repeats. Initialised to a colour we never see on cheerlights
lastRGB = (1,1,1)


def hex_to_rgb(hex):
    # converts a hex colour code into RGB
    h = hex.lstrip('#')
    r, g, b = (int(h[i:i + 2], 16) for i in (0, 2, 4))
    return r, g, b

# MQTT callback
def on_message(topic, msg):
    global msgCounter, isIdle, lastRGB, checkCount
    msgCounter += 1
    print((topic, msg, msgCounter))
    try:
        msgStr = msg.decode('ASCII')
        colRGB = hex_to_rgb(msgStr)
        print(f"msgStr: {msgStr}, colRGB: {colRGB}")
        
        # flash the onboard LED after getting data
        led.value(True)
        time.sleep(0.2)
        led.value(False)
        
        # When we reconnect to MQTT we pick up the same message as last time we connected, so to
        # avoid repeated setting in the colour history, we check if the colour has changed and
        # only update the history if it is different
        if colRGB == lastRGB:
            print(f"Repeat of last colour seen {lastRGB} so don't update pixels.")
        else:
            # add the new hex colour to the end of the array
            colour_array.append(colRGB)
            print(f'Colour added to array: {msgStr}')
            # remove the oldest colour in the array
            colour_array.pop(0)
            update_leds()

        
        # Store colour to check against next time
        lastRGB = colRGB
        
        #Reset message check counter
        checkCount = 0
        
    except ValueError as e:
        print(f"Invalid message format: {e}")
        

def reconnectMQTT():
    global client,checkCount
    # Reset check counter on connection reset
    checkCount = 0

    client = connect_mqtt.establishConnection('my_unique_client_id',led)
    if client != None:
        client.set_callback(on_message)
        client.subscribe(mqtt_topic)
        print(f"Subscribed to topic '{mqtt_topic}'")
    else:
        print(f"Failed to connect to MQTT network, will retry in 30 seconds.")
        machine.idle() # save power while waiting
        time.sleep(30)

def update_leds():
    # light up the LEDs
    # this step takes a second, it's doing a lot of hex_to_rgb calculations!
    print("Updating LEDs...")
    i = 0
    for x in range(width):
        for y in range(height):
            r = colour_array[i][0]
            g = colour_array[i][1]
            b = colour_array[i][2]
            current_colour = graphics.create_pen(r, g, b)
            graphics.set_pen(current_colour)
            graphics.pixel(x, y)
            i = i + 1
    gu.update(graphics)
    print("LEDs updated!")


# Create variable to hold reference to MQTT server once we have connected
client = None

gu = GalacticUnicorn()
graphics = PicoGraphics(DISPLAY)

width = GalacticUnicorn.WIDTH
height = GalacticUnicorn.HEIGHT

gu.set_brightness(0.5)
print(f"Brightness set to {gu.get_brightness()}")

current_colour = graphics.create_pen(0, 0, 0)

# set up an list to store the colours
colour_array = [(0,0,0)] * 583

# Set up status indicator and flash 5 times to show code started running
led = machine.Pin("LED", machine.Pin.OUT)
led.off()
for i in range(5):
    led.toggle()
    time.sleep(0.1)
led.toggle()

update_leds()

msgchkcounter = 0
while True:
    # adjust brightness with LUX + and -
    if gu.is_pressed(GalacticUnicorn.SWITCH_BRIGHTNESS_UP):
        gu.adjust_brightness(+0.04)
        update_leds()
        print(f"Brightness set to {gu.get_brightness()}")

    if gu.is_pressed(GalacticUnicorn.SWITCH_BRIGHTNESS_DOWN):
        gu.adjust_brightness(-0.04)
        update_leds()
        print(f"Brightness set to {gu.get_brightness()}")

    if msgchkcounter >= MQTTCHECK_INTERVAL * 100:
        msgchkcounter = 0
    
    if msgchkcounter == 0:
        # Check for MQTT messages
        checkCount += 1
        if checkCount > 90:
            print(f"{checkCount} No new messages detected for 30 minutes, resetting MQTT client.")
            reconnectMQTT()
        else:
            print(f"{checkCount} Checking for new messages")
        try:
            client.check_msg()
        except (AttributeError,OSError) as e:
            print(f"Exception in checking message: {e}")
            # Reconnect to MQTT server
            reconnectMQTT()
    msgchkcounter += 1
    
    # Pause briefly to allow processor breathing space   
    time.sleep(0.01)
