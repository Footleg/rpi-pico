set(LIBNAME "lsm6ds3")
add_library(${LIBNAME} lsm6ds3.cpp)

target_link_libraries(${LIBNAME} 
    pimoroni_i2c
)
