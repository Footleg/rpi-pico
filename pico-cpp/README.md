# Pico C++ Programs

Welcome to my C++ projects for devices using the RP2040 and RP2350 families of chips from Raspberry Pi. These are microcontroller chips used on the Raspberry Pi Pico, PicoW, Pico2 and Pico2W boards. I have reorganised these projects into folders by hardware required. These are mostly hardware products made and sold by [Pimoroni](https://shop.pimoroni.com). Each folder is designed to be opened individually in the VSCode editor to use the Raspberry Pi Pico extension to compile all the projects for that hardware combination together (generating one uf2 file per program).

Many of these projects use the Pico libraries from Pimoroni, and some use my own RGB Matrix Animations library to generate animations. If you cloned this repo without the --recursive switch then you will need to run the following command to clone the submodules to pull down the source code for these dependencies:

`git submodule update --init --recursive`

## Setting up a development environment for Pico C++ development

Since the Raspberry Pi Pico extension for VSCode was released, development for the Pico has been greatly simplified. But at the time of writing, many online resources still refer to the more complicated set-up required before this extension became available. So here is my guide to converting existing examples to build under the VSCode Pico extension. I have tested this on Windows 10 and Windows 11. I expect it works equally well on the Raspberry Pi or other Linux environments, but I have not tested this myself at this stage.

- Start by launching VSCode, then in the Extensions panel, search for the Raspberry Pi Pico extension and install it.

If you just want to build and play with one of my applications, these are already set up to build using the pico extension in VSCode. Just open the project folder in VSCode and click 'compile' in the task bar. See the README file in each project directory for further details on that project.

The rest of this guide covers how to start a new project which uses the Pimoroni libraries. We will start from the Pimoroni Boilerplate project, a template for setting up a new Pico project. This boilerplate was written for the old development environment using [WSL on Windows](https://learn.pimoroni.com/article/pico-development-using-wsl), we need to convert it to use the Raspberry Pi Pico extension.

- Download the boilerplate project from github: [Pimoroni Pico Boilerplate](https://github.com/pimoroni/pico-boilerplate). We want a copy not linked to the Pimoroni repo, so download the code as a zip file rather than checking it out using git.
- Unzip the boilerplate project into a new directory, and then rename this to the name you want to give your project.
- We also need the pimoroni pico libraries for this project, so these need checking out alongside your project folder.
(e.g. In the parent folder, do `git clone https://github.com/pimoroni/pimoroni-pico`)
- Now open the project folder in VSCode. A pop-up should appear asking if you want to import this project as a Raspberry Pi Pico project. Confirm this, and VSCode should open an import form.
- Accept the defaults and click ‘Import’.

The Pico extension will now configure things in the background. If this is the first time you have use it on this computer, then it will download the Pico SDK version selected, and the gcc compiler for the ARM processor. On subsequent loads, the these components will be located so it will be quicker than this first time. At the bottom of the VSCode window, you should see pico related controls. You can change the version of the Pico SDK here, and choose which flavour of Pico you are developing on. e.g. If you have a Pico2, then change the Board to pico2.

Now this is where things didn't go quite so smoothly on Windows. The project may not compile yet, because something isn't quite right in the CMake configuration. Some of the includes fail to be located. The cpp source files will show source not found errors on the includes. You can fix this with the CMakeTools extension.

- On the Extensions panel in VSCode, search for CMakeTools and install this extension.
- Open the panel for the CMakeTools extension, look in the Configure group. If it says 'No kit selected', click on the pencil icon next to this and select the 'Pico' kit. If this kit is not present in the list then select 'scan for kits', then try again.
- With a suitable kit selected, you can now click the ellipsis button in the header of the 'Project Outline' group, and select 'Clean Reconfigure All Projects' from the menu. The Project Outline pane should be populated now, and the main.cpp source file in the code editor should no longer show any errors for the #includes.
- Click the compile button in the toolbar, and your project should build successfully.
- Check the build directory under your project directory now contains a uf2 file. This is the built binary which you can flash onto your Pico (but don't as this was just an example requiring hardware you may not have).

If you are working under this repo with my examples, and you checked out the repo some time in the past then you may not have pulled in the latest version of the pimoroni-pico libraries. Just doing a git pull does not update the submodules (other repos references by this repo). Failing to realise this cost me an entire weekend of pain trying to get code to build. So make sure you have the most up to date copies of the submodules by running this command:
`git submodule update --init --recursive`

That hopefully fixes any compile errors in the boilerplate project.

Now you can start working on your own Pico C++ projects, replacing the code in the main.cpp with your own code. You will need to include libraries in the CMakeLists.txt which your code uses. If you are working with some of the [Pimoroni Pico products](https://shop.pimoroni.com/collections/pico), then the best starting point is one of their examples. So here is a walk through of building one of these examples in your new project folder using the VSCode Pico extension.

- Browse through the Pimoroni examples in the pimoroni-pico repo folder you already created alongside your project folder. Here I'll pick one of the galactic unicorn examples from pimoroni-pico/examples/galactic_unicorn
- Open the CMakeLists.txt file from this folder in a text editor, and look for the 'fire effect' example.
- Note which cpp file the example uses (fire_effect.cpp), and copy this into your project folder.
- Check the includes in fire_effect.cpp. It includes another source file from the example folder (okcolor.hpp), so we also need to copy that file into our project folder. The other includes are pimoroni-pico libraries we will need to include via the CMakeLists file.
- From the examples folder CMakeLists.txt file, copy the list of target_link_libraries(fire_effect pico_stdlib hardware_pio hardware_adc hardware_dma pico_graphics galactic_unicorn) used by the fire effect example. Paste these into the CMakeLists.txt file in your project in VSCode, replacing the set used by the original sample code. Take out the 'fire_effect' item, and replace this with ${NAME} at the start of the list, as our CMakeLists file uses this NAME variable to ensure the same name is used throughout the configuration.
- Edit the name in the block: set(NAME pico-boilerplate) to make this project your own. (Change pico-boilerplate to a name of your choice).
- Edit the source file specified for the project in the add_executable block, changing main.cpp to fire_effect.cpp
- If we try to compile now, we will get errors as some required libraries are not included. We need to figure out the includes needed in the CMakeLists.txt file for the target libraries we included (and remove those from the boilerplate that we do not need). The includes in the cpp source files give some clues, as do the target link libraries. If you miss any, then the compiler errors will also give you some ideas as to what was missed. For further guidance, take a look in the CMakeLists.txt files under any of my example project folders in this repo for different Pimoroni Pico hardware combinations.

For the fire_effect example, I determined we needed this list:

- include(common/pimoroni_bus)
- include(libraries/bitmap_fonts/bitmap_fonts)
- include(libraries/hershey_fonts/hershey_fonts)
- include(libraries/galactic_unicorn/galactic_unicorn)
- include(libraries/pico_graphics/pico_graphics)

This should be everything needed to compile the example and produce a uf2 file to flash onto a [glacticunicorn](https://shop.pimoroni.com/products/space-unicorns?variant=40842033561683) from Pimoroni. Look for a uf2 file matching the name you gave your project in the CMakeLists.txt file when you made it your own. If you have a different board, try to repeat the above steps with an example for the board you have.

For further examples of ready configured projects to build using the VSCode Raspberry Pi Pico Extension, take a look at the various folders in this repo for each hardware setup. I have extended the boilerplate CMakeLists files to include additional projects via cmake includes. See the Tufty2040 folder for example. I add each project via a separate cmake file to specify the source file to build it and the target link libraries for that project. But they share the same set of includes for the hardware in the main CMakeLists.txt file for that folder. Each additional appliation is added via an include of the .cmake file into the main CMakeLists.txt file. This allows one configuration for the hardware to build multiple applications, generating one uf2 file per project from the one CMake configuration. Any additional libraries used by an application can be included from the .cmake file for that application. e.g. the tufty2040_animations application shows how I include my RGBMatrixAnimations library source to be compiled into the project.
