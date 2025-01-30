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
set(PICO_BOARD pico2 CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.12)

# Change your executable name to something creative!
set(NAME pico_display28_btns) # <-- Name your project/executable here!

include(pico_sdk_import.cmake)
include(pimoroni_pico_import.cmake)

# Gooey boilerplate
project(${NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

# Add your source files
add_executable(${NAME}
    pico_display_28_buttons.cpp # <-- Add source files here!
    mybutton.cpp
)

# Include required libraries
# This assumes `pimoroni-pico` is stored alongside your project
include(common/pimoroni_bus)
include(libraries/pico_display_28/pico_display_28)
include(libraries/pico_graphics/pico_graphics)
include(drivers/button/button)
include(drivers/rgbled/rgbled)
include(drivers/st7789/st7789)

# Don't forget to link the libraries you need!
target_link_libraries(${NAME} # <-- List libraries here!
	pico_stdlib 
	hardware_spi 
	hardware_pwm 
	hardware_dma 
  hardware_pio
  hardware_adc
  rgbled 
  button 
	pico_display_28 
	st7789 
	pico_graphics
)

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

include(pico_display_28_balls.cmake)
