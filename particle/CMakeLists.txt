
add_library(particle STATIC particle.cpp effects.cpp)
target_include_directories(particle PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)
target_link_libraries(particle PRIVATE raylib)


# find_package(PkgConfig REQUIRED)
# pkg_check_modules(LUAJIT REQUIRED luajit)
#
# add_executable(particle_system particle_system.cpp)
# target_include_directories(particle_system PRIVATE ${LUAJIT_INCLUDE_DIRS})
# target_link_libraries(particle_system PRIVATE particle raylib ${LUAJIT_LIBRARIES})

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
find_package(Julia 1.9.0 REQUIRED)

message(STATUS Julia)

add_executable(particle_system particle_system.cpp)
target_include_directories(particle_system PRIVATE ${JULIA_INCLUDE_DIR})
target_link_libraries(particle_system PRIVATE particle raylib ${JULIA_LIBRARY})

foreach(target IN ITEMS particle particle_system)
    target_compile_options(${target} PRIVATE ${COMMON_COMPILE_OPTIONS})
    target_link_options(${target} PRIVATE ${COMMON_LINK_OPTIONS})
endforeach()

target_link_options(particle_system PRIVATE -rdynamic)
