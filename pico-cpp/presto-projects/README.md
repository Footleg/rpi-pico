# Presto C++ Projects

A basic project for getting started with C++ on the RP2350-powered Presto.

To build this, you need to check out the Pimoroni Presto repo, and the Pimoroni
Pico repos in the same parent folder as the Presto Projects repo. This is most
easily done by checking out the entire rpi-pico repo from github using the
--recursive flag to fetch all the required repos as submodules:

git clone https://github.com/Footleg/rpi-pico.git --recursive

Then open the presto-projects folder in VSCodem and using the CMake Tools extension,
at the top of the CMake Tools pane click the 'Delete Cache and reconfigure' icon. 
Then you can compile using the VSCode pico extension.
