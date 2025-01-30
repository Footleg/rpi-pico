set(OUTPUT_NAME pico_display_28_balls)
add_executable(${OUTPUT_NAME} 
  pico_display_28_balls.cpp # <-- Add source files here!
  mybutton.cpp
)

target_link_libraries(${OUTPUT_NAME} # <-- List libraries here!
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

# enable usb output
pico_enable_stdio_usb(${OUTPUT_NAME} 1)

pico_add_extra_outputs(${OUTPUT_NAME})
