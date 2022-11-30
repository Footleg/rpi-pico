"""
NeoPixel Cheerlights MQTT listener.

Copyright (c) 2022 Dr Footleg

A simple demonstration of MQTT using the connect_mqtt module.
This program connects to a wifi network and then listens for messages on
the cheerlights MQTT server. For each change in the cheerlights colour,
it updates the LEDs on a Neopixel strip, showing the history of colours
received over time. It will automatically attempt to reconnect to the same
MQTT server if any (handled) network exceptions occur.

REQUIRED HARDWARE:
* PicoW
* RGB NeoPixel LEDs connected to pin GP0.

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
from neopixel import Neopixel
MQTTCHECK_INTERVAL = 20  # Interval in seconds between checks for MQTT messages

mqtt_topic = 'cheerlightsRGB'

# Update this to match the number of NeoPixel LEDs connected to your board.
num_pixels = 24

# Counter for the number of messages received
msgCounter = 0
checkCount = 0

# Track last colour seen to avoid repeats. Initialised to a colour we never see on cheerlights
lastRGB = (1,1,1)

#Initialise neopixels
pixels = Neopixel(num_pixels, 0, 0, "GRB")
pixels.brightness(10)

# MQTT callback
def on_message(topic, msg):
    global msgCounter, isIdle, lastRGB
    msgCounter += 1
    print((topic, msg, msgCounter))
    try:
        msgStr = msg.decode('ASCII')
        colBytes = bytearray.fromhex(msgStr[1] + msgStr[2] + msgStr[3] + msgStr[4] + msgStr[5] + msgStr[6])
        colRGB = struct.unpack('<BBB', colBytes)
        print(f"msgStr: {msgStr}, colBytes: {colBytes}, colRGB: {colRGB}")
        
        # When we reconnect to MQTT we pick up the same message as last time we connected, so to
        # avoid repeated setting in the colour history, we check if the colour has changed and
        # only update the history if it is different
        if colRGB == lastRGB:
            print(f"Repeat of last colour seen {lastRGB} so don't update neopixels.")
        else:
            # Move colour 1 step along chain of pixels
            for i in range( 1, num_pixels ):
                lastRGB = pixels.get_pixelRGB(num_pixels - i - 1)
                pixels.set_pixel(num_pixels - i, lastRGB )
            
        # Set first pixel to current colour
        pixels.set_pixel(0, colRGB )
        pixels.show()
        
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

# Create variable to hold reference to MQTT server once we have connected
client = None

# Set up status indicator and flash 5 times to show code started running
led = machine.Pin("LED", machine.Pin.OUT)
led.off()
for i in range(5):
    led.toggle()
    time.sleep(0.1)
led.toggle()

msgchkcounter = 0
while True:
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
