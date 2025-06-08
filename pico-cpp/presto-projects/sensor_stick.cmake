set(OUTPUT_NAME sensor_stick)

add_executable(${OUTPUT_NAME} 
  src/sensorstick.cpp # <-- Add source files here!
)

target_link_libraries(${OUTPUT_NAME} # <-- List libraries here!
  st7701_presto
  pico_stdlib
  pico_multicore
  pimoroni_i2c
  hardware_interp
  hardware_adc
  pico_graphics
  lsm6ds3
)

# Enable USB UART output only
pico_enable_stdio_uart(${OUTPUT_NAME} 0)
pico_enable_stdio_usb(${OUTPUT_NAME} 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(${OUTPUT_NAME})
