"""
MQTT connection class

Copyright (c) 2022 Dr Footleg

This class establishes a wifi connection and a connection to an MQTT server.
The wifi network and MQTT server details are read from a secrets file. This
can contain multiple networks and MQTT servers, and the class will attempt to
connect to the first network it can see which matches an entry in the secrets
file. By supporting multiple networks, you can run code on your board that can
connect to your home network when you are at home, or your maker space network
or work network when you are there. It will attempt to connect to the MQTT
servers in the secrets file in turn until it is successfull. This allows you
for example to connect in preference to a local server on your network, but fall
back to use a server on the internet if you are not connected to your home network.

REQUIRES:
 - umqtt.simple: https://mpython.readthedocs.io/en/master/library/mPython/umqtt.simple.html 

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
import network
import time
import machine
from umqtt.simple import MQTTClient
from secrets import secrets
import json

secrets_obj = json.loads(secrets)
mqtt_server_index = -1

def establishConnection(client_id,led):
    global mqtt_server_index
    
    mqtt_server = ''
    mqtt_port = 0
    user_t = ''
    password_t = ''


    def mqtt_connect():
        if user_t == '':
            print(f'Trying MQTT server {mqtt_server}:{mqtt_port} without credentials')
            client = MQTTClient(client_id, mqtt_server, port=mqtt_port)
        else:
            print(f'Trying MQTT server {mqtt_server}:{mqtt_port} with credentials')
            client = MQTTClient(client_id, mqtt_server, port=mqtt_port, user=user_t, password=password_t) 
        client.connect()
        print(f'Connected to {mqtt_server} MQTT Broker')
        return client

    # Establish Wi-fi connection
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    #wlan.disconnect()

    print('Scanning for networks...')
    nets = wlan.scan()
    print(f'Found {len(nets)} networks')
    for net in nets:
        print(f"Found ssid: {net[0]}")
        for known_net in secrets_obj['wifi']:
            print(f"Matching with: {known_net['ssid']}")
            if net[0] == str.encode( known_net['ssid'] ):
                print('Network found!')
                wlan.connect(net[0], known_net['password'])
                tries = 0
                while not wlan.isconnected() and tries < 200:
                    print('Not yet connected')
                    tries += 1
                    led.off()
                    for i in range(6):
                        led.toggle()
                        time.sleep(0.2)
                    machine.idle() # save power while waiting
                    time.sleep(1)
                if wlan.isconnected():
                    print(f'WLAN connection succeeded in {tries} attempts!')
                    break
                else:
                    print(f'Failed to connect in {tries} attempts')
        if wlan.isconnected():
            break

    # Determine if wifi connection was made
    if wlan.isconnected():
        # Connect to MQTT server
        mqttConnected = False
        tries = 0
        while not mqttConnected and tries < 5:
            tries += 1
            led.off()
            for i in range(4):
                led.toggle()
                time.sleep(0.2)
            machine.idle() # save power while waiting
            time.sleep(1)

            # Check if we already found a server
            if mqtt_server_index >= 0:
                mqtt_server = secrets_obj['mqtt'][mqtt_server_index]['server']
                try:
                    mqtt_port = int(secrets_obj['mqtt'][mqtt_server_index]['port'])
                except KeyError as e:
                    mqtt_port = 0
                user_t = secrets_obj['mqtt'][mqtt_server_index]['username']
                password_t = secrets_obj['mqtt'][mqtt_server_index]['password']
                try:
                    client = mqtt_connect()
                    mqttConnected = True
                except OSError as e:
                    print('Failed to connect to MQTT Broker. Reconnecting...')
                    time.sleep(1)
            else:
                server_idx = -1
                for server in secrets_obj['mqtt']:
                    server_idx += 1
                    mqtt_server = server['server']
                    try:
                        mqtt_port = int(server['port'])
                    except KeyError as e:
                        mqtt_port = 0
                    user_t = server['username']
                    password_t = server['password']
                    try:
                        client = mqtt_connect()
                        mqttConnected = True
                        mqtt_server_index = server_idx
                        break;
                    except OSError as e:
                        print('Failed to connect to MQTT Broker. Reconnecting...')
                        time.sleep(1)

                
        if mqttConnected:
            # Return client
            return client
        else:
            # No MQTT connection, indicate with rapid double flash of LED
            # repeated 20 times before returning to calling program
            led.off()
            print('Failed to connect to MQTT Broker. Pausing to flash indicator signal before next attempt.')
            for i in range( 20 ):
                for i in range(3):
                    led.toggle()
                    time.sleep(0.1)
                led.toggle()
                machine.idle() # save power while waiting
                time.sleep(1)
                
            return None
    else:
        # No wifi connection, indicate with rapid flash of LED
        print('Failed to connect to a WiFi network. Restart Pico to retry.')
        while True:
            led.toggle()
            time.sleep(0.1)

