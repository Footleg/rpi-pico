# Cheerlights MQTT

This example for the PicoW subscribes to the Cheerlights MQTT server, and sets a pixel on a neopixel strip each time the Cheerlights colour changes.

It requires the umqtt.simple library, and this fork of the Neopixel library <https://github.com/Footleg/pi_pico_neopixel/blob/main/neopixel.py>

Update the secrets file with details of the wifi network(s) you want it to try and connect to. It will attempt to connect to any of the networks
it can see from the list you provide, allowing you to program a PicoW to connect to networks in different locations you might be using it without
having to reprogram it for each location.

To run the main.py program on a PicoW, I installed the umqtt.simple library via the Thonny editor (Tools/Manage Packages), and then copied the neopixel.py file onto the root of the PicoW. Copy all the python files from this folder onto the PicoW and you should be able to run the example.

The example outputs information to the console in Thonny, and flashes the on board LED to indicate connection status. To see the colours on some Neopixels,
connect them to the PicoW following the guide in the <https://github.com/Footleg/pi_pico_neopixel> repo. You can run the example without neopixels if you are just testing and plan to add some alternative indicators.
