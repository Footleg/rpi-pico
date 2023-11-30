"""
Example program to run on the host computer to send data to a connected Pico board

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
import time
import os
import serial

def sendByte(serialconn, value):
    # Convert to hex and send as ascii
    hexVal = hex(value)
    if value < 16:
        chars = "0" + hexVal[2]
    else:
        chars = hexVal[2] + hexVal[3]
    print(f"Hex: {hexVal}, Chars: {chars}")
    ser.write(bytes(chars,"ascii"))

if os.path.exists("/dev/ttyACM0"):
    ser = serial.Serial("/dev/ttyACM0",115200)
    print("Connected")

    # Send full range of byte values as hex
    for i in range(256):
        sendByte(ser,i)
        time.sleep(0.1)

    for i in range(5):
        # Send the code to hold the LED on
        ser.write(bytes('l',"ascii"))
        time.sleep(1)

    # Send a series of bytes as ascii
    ser.write(bytes('ABCDEFGH',"ascii"))
    time.sleep(0.5)

    # Send a set of bytes as numeric values
    ser.write(bytes([109,43,130]))

print("Ended")
