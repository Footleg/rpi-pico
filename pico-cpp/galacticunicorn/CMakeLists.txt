# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.12)

# Change your executable name to something creative!
set(NAME galacticunicorn-animations) # <-- Name your project/executable here!

include(pico_sdk_import.cmake)
include(pimoroni_pico_import.cmake)

# Gooey boilerplate
project(${NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

include_directories("${PROJECT_SOURCE_DIR}/../RGBMatrixAnimations/src") # Include additional source directory
add_subdirectory(../RGBMatrixAnimations/src RGBMatrixAnimations) # Map additional source to RGBMatrixAnimations library name

# Add your source files
add_executable(${NAME}
    galunicorn_anim.cpp # <-- Add source files here!
)

# Include required libraries
# This assumes `pimoroni-pico` is stored alongside your project
include(common/pimoroni_bus)
include(libraries/bitmap_fonts/bitmap_fonts)
include(libraries/hershey_fonts/hershey_fonts)
include(libraries/galactic_unicorn/galactic_unicorn)
include(drivers/button/button)

# Don't forget to link the libraries you need!
     
target_link_libraries(${NAME} 
    pico_stdlib
    galactic_unicorn
    hardware_spi
    hardware_dma
    hardware_pio
    hardware_adc
    button
    pimoroni_bus 
    hardware_pwm 
    RGBMatrixRenderer
    Crawler
    GameOfLife
    GravityParticles
)

# enable usb output
pico_enable_stdio_usb(${NAME} 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(${NAME})

# Set up files for the release packages
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.uf2
    ${CMAKE_CURRENT_LIST_DIR}/README.md
    DESTINATION .
)

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_GENERATOR "ZIP" "TGZ")
include(CPack)
