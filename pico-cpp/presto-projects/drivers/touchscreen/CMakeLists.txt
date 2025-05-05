set(LIBNAME "touchscreen")
add_library(${LIBNAME} touchscreen.cpp)

target_link_libraries(${LIBNAME} 
    pimoroni_i2c
)
