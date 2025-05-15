cmake_minimum_required(VERSION 3.21)
project("Manalter" VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE Debug)

# add_compile_options(
#     -Wall -Wextra -Wuninitialized -Wmissing-field-initializers
#     -Wno-unused-variable -Wshadow -Wconversion -Wsign-conversion
#     -Wdouble-promotion -Wnull-dereference -Wold-style-cast -Woverloaded-virtual -Wnon-virtual-dtor -O3 -flto -ffunction-sections -fdata-sections)
#     # -fsanitize=undefined -fsanitize-undefined-trap-on-error -fno-sanitize-recover=undefined -g -fno-omit-frame-pointer)
# # add_link_options(-fsanitize=undefined -fsanitize-undefined-trap-on-error -fno-sanitize-recover=undefined -g -fno-omit-frame-pointer)
# add_link_options(-O3 -flto -Wl,--gc-sections -s)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_LINKER lld)

add_subdirectory(src)

option(TEST "" OFF)

if (TEST)
    add_subdirectory(test)
    set_target_properties(tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()
