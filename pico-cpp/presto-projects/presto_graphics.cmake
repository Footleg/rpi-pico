set(OUTPUT_NAME presto_graphics)

add_executable(${OUTPUT_NAME} 
  src/presto_graphics.cpp # <-- Add source files here!
)

target_link_libraries(${OUTPUT_NAME} # <-- List libraries here!
  st7701_presto
  pico_stdlib
  pico_multicore
  pimoroni_i2c
  ft6x36
  touchscreen
  sdcard
  fatfs
  hardware_interp
  hardware_adc
  pico_graphics
  footleg_graphics
)

# Enable USB UART output only
pico_enable_stdio_uart(${OUTPUT_NAME} 0)
pico_enable_stdio_usb(${OUTPUT_NAME} 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(${OUTPUT_NAME})
