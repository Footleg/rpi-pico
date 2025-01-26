# Pico C++ Programs

Welcome to my C++ projects for devices using the RP2040 and RP2350 families of chips from Raspberry Pi. These are microcontroller chips used on the Raspberry Pi Pico, PicoW, Pico2 and Pico2W boards. I have reorganised these projects into folders by hardware required. These are mostly product made and sold by [Pimoroni](https://shop.pimoroni.com). Each folder is designed to be opened individually in the VSCode editor to use the Raspberry Pi Pico extension to compile all the projects for that hardware combination together (generating one uf2 file per program).

Many of these projects use the Pico libraries from Pimoroni, and some use my own RGB Matrix Animations library to generate animations. If you cloned this repo without the --recursive switch then you will need to run the following command to clone the submodules to pull down the source code for these dependencies:

`git submodule update --init --recursive`

## Setting up a development environment for Pico C++ development

Since the Raspberry Pi Pico extension for VSCode was released, development for the Pico has been greatly simplified. But at the time of writing, many online resources still refer to the more complicated set-up required before this extension became available. So here is my guide to converting existing examples to build under the VSCode Pico extension. I have tested this on Windows 10 and Windows 11. I expect it works equally well on the Raspberry Pi or other Linux environments, but I have not tested this myself at this stage.

- Start by launching VSCode, then in the Extensions panel, search for the Raspberry Pi Pico extension and install it.

Now we are going to import an existing project which uses the Pimoroni libraries. This project is the Pimoroni Boilerplate project, a template for setting up a new Pico project. As it was written for the old development environment using [WSL on Windows](https://learn.pimoroni.com/article/pico-development-using-wsl), we need to convert it to use the Raspberry Pi Pico extension.

- Download the boilerplate project from github: [Pimoroni Pico Boilerplate](https://github.com/pimoroni/pico-boilerplate). We want a copy not linked to the Pimoroni repo, so download the code as a zip file rather than checking it out using git.
- Unzip the boilerplate project into a new directory, and then rename this to the name you want to give your project.
- We also need the pimoroni pico libraries for this project, so these need checking out alongside your project folder. 
(e.g. In the parent folder, do 'git clone https://github.com/pimoroni/pimoroni-pico')
- Now open the project folder in VSCode. A pop-up should appear asking if you want to import this project as a Raspberry Pi Pico project. Confirm this, and VSCode should open an import form.
- Accept the defaults and click ‘Import’.

The Pico extension will now configure things in the background. If this is the first time you have use it on this computer, then it will download the Pico SDK. On subsequent loads, the SDK will be located so it will be quicker than this first time. At the bottom of the VSCode window, you should see pico related controls. You can change the version of the Pico SDK here, and choose which flavour of Pico you are developing on. e.g. If you have a Pico2, then change the Board to pico2. 

Now this is where things didn't go quite so smoothly on Windows. The project will not compile yet, because something isn't quite right in the CMake configuration. Some of the includes fail to be located. The cpp source files will also show source not found errors on the includes. You can fix this with the CMakeTools extension and a seperate install of an earlier version of the arm-none-eabi compiler:

- Download the AArch32 bare-metal target (arm-none-eabi) executable (.exe) from <https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads>
To get this to work, I had to download v12.3 as none of the v13 or v14 builds I tried worked for me. The exe I downloaded was arm-gnu-toolchain-12.3.rel1-mingw-w64-i686-arm-none-eabi.exe
- Install the arm gnu toolchain, and on the final page of the install wizard, make sure you check the box to 'Add path to environment variable'.
- If you have VSCode open, you should now restart it so it picks up this change to the path environment variable.
- On the Extensions panel in VSCode, search for CMakeTools and install this extension.
- Open the panel for the CMakeTools extension, and in the Configure group, select 'scan for kits'.
- Once the scan is complete, you should see your GCC 12.3.1 arm-none-eabi in the kits list and you can select it.
- With a suitable kit version selected, you can now click the ellipsis button in the header of the 'Project Outline' group, and select 'Clean Reconfigure All Projects' from the menu.
If this step fails with an error, then change the kit selection to 'pico' and do Clean Reconfigure All Projects again. Then try to compile. I found this compile failed due to failing to find some dependencies, but I could now change back to the GCC 12.3.1 kit, do the Clean Reconfigure All Projects again (which succeeded this time), and now it could compile.
- Now finally, you should see things working. Open the main.cpp file in the code editor and it should not show any errors.
- Click the compile button in the toolbar, and your project should build successfully.
- Check the build directory under your project directory now contains a uf2 file. This is the built binary which you can flash onto your Pico (but don't as this was just an example requiring different hardware).

Now you can start working on your own Pico C++ projects, replacing the code in the main.cpp with your own code. You will need to include libraries in the CMakeLists.txt which your code uses. Here is a walk through of building one of the Pimoroni examples in your new project.
- Browse into the pimoroni-pico repo examples. Here I'll pick one of the galactic unicorn examples from pimoroni-pico/examples/galactic_unicorn
- Open the CMakeLists.txt file from this folder in a text editor, and look for the 'fire effect' example.
- Note which cpp file the example uses (fire_effect.cpp), and copy this into your project folder.
- Check the includes in fire_effect.cpp. It includes another source file from the example folder (okcolor.hpp), so we also need to copy that file into our project folder. The other includes are pimoroni-pico libraries we will need to include via the CMakeLists file.
- Copy the list of target_link_libraries(fire_effect pico_stdlib hardware_pio hardware_adc hardware_dma pico_graphics galactic_unicorn) used by this example. Paste these into the CMakeLists.txt file in your project in VSCode, replacing the set used by the original sample code. Take out the fire_effect item, and replace with ${NAME} at the start of the list, as our CMakeLists file uses this NAME variable to ensure the same name is used throughout.
- Edit the source file used in the the project in the add_executable block, changing main.cpp to fire_effect.cpp
- If we try to compile now, we will get errors as some required libraries are not included. We need to figure out the includes needed in the CMakeLists.txt file for the target libraries we included. The includes in the cpp source files give some clues, as do the target link libraries.

I determined we needed this list:
- include(common/pimoroni_bus)
- include(libraries/bitmap_fonts/bitmap_fonts)
- include(libraries/hershey_fonts/hershey_fonts)
- include(libraries/galactic_unicorn/galactic_unicorn)
- include(libraries/pico_graphics/pico_graphics)

This should be everything needed to compile the example and produce a uf2 file to flash onto a [glacticunicorn](https://shop.pimoroni.com/products/space-unicorns?variant=40842033561683) from Pimoroni.

For further examples of ready configured projects ready to buid using the VSCode Raspberry Pi Pico Extension, take a look at the various folders in this repo for each hardware setup. I have extended the boilerplate CMakeLists files to include additional projects via cmake includes. See the Tufty2040 folder for example. This allows one configuration to build multiple projects, generating one uf2 file per project from the one hardware configuration.
