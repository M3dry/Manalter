cmake_minimum_required(VERSION 3.21)
project("Manalter" VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE Debug)

add_compile_options(-O3 -Wall -Wextra -Wuninitialized -Wmissing-field-initializers -Wno-unused-variable)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_LINKER lld)

add_subdirectory(src)

add_subdirectory(test)
set_target_properties(tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
