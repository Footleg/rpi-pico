# Pico C++ Programs

## Setting up a Windows build environment for Pico C++ development

If you are working on a Windows computer, there are a couple of options for setting up a development environment to compile these C++ programs for the Pico.

- Using WSL: See this guide from Pimoroni on how to set this up. <https://learn.pimoroni.com/article/pico-development-using-wsl>
- Using a GIT bash shell. Follow this guide from Shawn Hymel <https://shawnhymel.com/2096/how-to-set-up-raspberry-pi-pico-c-c-toolchain-on-windows-with-vs-code/>

I went with the guide from Shawn Hymel using VSCode, but ran into a couple of snags which I solved with the help of the comments on his article. Search for 'Footleg' in the comments on that page for my tips to resolve the issues I found if things don't build for you.

I have since found I needed different syntax for the fix for the problem with the Error -1073741511. If you get this error, you need to add the following line to the .bashrc file (located in the C:\Users\\[your-username] folder on Windows): export PATH=/c/VSARM/mingw/mingw64/bin:$PATH
(Change that path to point to your mingw64 location if you installed it somewhere different). Remember to start a new git bash terminal after you update the bash.rc file for the changes to take effect.

If you cloned this repo without the --recursive switch then you will need to run the following command to clone the submodules:
`git submodule update --init --recursive`

You should now be able to build any of the projects from this repo, by following these steps.

- Create a 'build' directory in the project folder which you want to build.
- Enter the build directory and run the command `cmake -G "MinGW Makefiles" ..`
- Enter the command `make -j8` (Change 8 to the number of CPU cores on your computer for fastest compile times)

The first build can take a while as all the libraries we are using need to get compiled, but subsequent builds should be faster.
When it is done, there should be a .uf2 file in the build directory which you can drag and drop onto your Pico or other RP2040 device.
