set(OUTPUT_NAME presto_animations)
add_executable(${OUTPUT_NAME} src/presto_animations.cpp)

add_subdirectory(../RGBMatrixAnimations/src RGBMatrixAnimations)

include_directories("${PROJECT_SOURCE_DIR}/../RGBMatrixAnimations/src")

target_link_libraries(${OUTPUT_NAME}
  st7701_presto
  pico_stdlib
  pico_multicore
  pimoroni_i2c
  ft6x36
  touchscreen
  lsm6ds3
  hardware_interp
  hardware_adc
  pico_graphics
  footleg_graphics
  RGBMatrixRenderer
  Crawler
  GameOfLife
  GravityParticles
  sparkfun_pico
)

# enable usb output
pico_enable_stdio_usb(${OUTPUT_NAME} 1)

pico_add_extra_outputs(${OUTPUT_NAME})
