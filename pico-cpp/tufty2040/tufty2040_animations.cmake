set(OUTPUT_NAME tufty2040_animations)
add_executable(${OUTPUT_NAME} tufty2040_animations.cpp)

add_subdirectory(../RGBMatrixAnimations/src RGBMatrixAnimations)

include_directories("${PROJECT_SOURCE_DIR}/../RGBMatrixAnimations/src")

target_link_libraries(${OUTPUT_NAME}
    pico_stdlib
    tufty2040
    hardware_spi
    hardware_dma
    hardware_pio
    hardware_adc
    pico_graphics
    st7789
    button
    pimoroni_bus 
    hardware_pwm 
    RGBMatrixRenderer
    Crawler
    GameOfLife
    GravityParticles
)

# enable usb output
pico_enable_stdio_usb(${OUTPUT_NAME} 1)

pico_add_extra_outputs(${OUTPUT_NAME})
