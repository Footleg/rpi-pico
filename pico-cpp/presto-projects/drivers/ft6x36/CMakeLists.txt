set(LIBNAME "ft6x36")
add_library(${LIBNAME} ft6x36.cpp)

target_link_libraries(${LIBNAME} 
    pimoroni_i2c
    pico_graphics
)
