set(LIBNAME "footleg_graphics")
add_library(${LIBNAME} footleg_graphics.cpp)

target_link_libraries(${LIBNAME} 
    pico_graphics
)
