cmake_minimum_required(VERSION 3.21)
project("Manalter" VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_LINKER lld)

set(COMMON_COMPILE_OPTIONS
    -Wall -Wextra -Wuninitialized -Wmissing-field-initializers
    -Wno-unused-variable -Wshadow -Wconversion -Wsign-conversion
    -Wdouble-promotion -Wnull-dereference -Woverloaded-virtual -Wnon-virtual-dtor
    -O3 -ffunction-sections -fdata-sections
)
set(COMMON_LINK_OPTIONS
    -O3 -Wl,--gc-sections -s
)

add_subdirectory(glad)
add_subdirectory(src)
add_subdirectory(particle)
add_subdirectory(mote)
add_subdirectory(julip)
add_subdirectory(ecs)

option(TEST "" OFF)

if (TEST)
    add_subdirectory(test)
    set_target_properties(tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()
