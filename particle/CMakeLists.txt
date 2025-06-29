add_library(particle STATIC particle.cpp effects.cpp)
target_include_directories(particle PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)
target_link_libraries(particle PRIVATE raylib)

add_executable(particle_system particle_system.cpp)
target_link_libraries(particle_system PRIVATE particle raylib lua)

foreach(target IN ITEMS particle) # particle_system)
    target_compile_options(${target} PRIVATE ${COMMON_COMPILE_OPTIONS})
    target_link_options(${target} PRIVATE ${COMMON_LINK_OPTIONS})
endforeach()
