set(MANALTER_LIB_SOURCES
    assets.cpp
    hitbox.cpp
    loop.cpp
    loop_wrapper.cpp
    rayhacks.cpp
    spell.cpp
    spell_caster.cpp
    utility.cpp
    enemies.cpp
    enemies_spawner.cpp
    item_drops.cpp
    player.cpp
    ui.cpp
    input.cpp
    power_up.cpp
    particle_system.cpp
    effects.cpp
    font.cpp
)

set(HITBOX_DEMO_SOURCES
    hitbox_demo.cpp
)

set(QUADTREE_DEMO_SOURCES
    quadtree_demo.cpp
)

set(PARTICLE_SYSTEM_DEMO_SOURCES
    particle_system_demo.cpp
)

set(PLAYGROUND_SOURCES
    playground.cpp
)

set(COMMON_COMPILE_OPTIONS
    -Wall -Wextra -Wuninitialized -Wmissing-field-initializers
    -Wno-unused-variable -Wshadow -Wconversion -Wsign-conversion
    -Wdouble-promotion -Wnull-dereference -Wold-style-cast -Woverloaded-virtual -Wnon-virtual-dtor
    -O3 -ffunction-sections -fdata-sections
)

set(COMMON_LINK_OPTIONS
    -O3 -Wl,--gc-sections -s
)

add_library(manalter_lib STATIC ${MANALTER_LIB_SOURCES})
target_compile_options(manalter_lib PRIVATE ${COMMON_COMPILE_OPTIONS})
target_link_options(manalter_lib PRIVATE ${COMMON_LINK_OPTIONS})
target_include_directories(manalter_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(manalter_lib PRIVATE raylib)

add_library(manalter_lib_debug STATIC ${MANALTER_LIB_SOURCES})
target_compile_options(manalter_lib_debug PRIVATE ${COMMON_COMPILE_OPTIONS})
target_link_options(manalter_lib_debug PRIVATE ${COMMON_LINK_OPTIONS})
target_include_directories(manalter_lib_debug PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(manalter_lib_debug PRIVATE raylib)
target_compile_definitions(manalter_lib_debug PUBLIC DEBUG)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_options(manalter_lib PRIVATE mwindows)
    target_link_libraries(manalter_lib PRIVATE winmm "stdc++exp")

    target_link_options(manalter_lib_debug PRIVATE mwindows)
    target_link_libraries(manalter_lib_debug PRIVATE winmm "stdc++exp")
endif()

add_executable(manalter main.cpp)
target_link_libraries(manalter PRIVATE manalter_lib)

add_executable(manalter_debug main.cpp)
target_link_libraries(manalter_debug PRIVATE manalter_lib_debug)

add_executable(hitbox_demo ${HITBOX_DEMO_SOURCES})
target_link_libraries(hitbox_demo PRIVATE manalter_lib)

add_executable(quadtree_demo ${QUADTREE_DEMO_SOURCES})
target_link_libraries(quadtree_demo PRIVATE manalter_lib)

add_executable(particle_system_demo ${PARTICLE_SYSTEM_DEMO_SOURCES})
target_link_libraries(particle_system_demo PRIVATE manalter_lib)

add_executable(playground ${PLAYGROUND_SOURCES})
target_link_libraries(playground PRIVATE manalter_lib)

foreach(target IN ITEMS manalter manalter_debug hitbox_demo playground)
    target_compile_options(${target} PRIVATE ${COMMON_COMPILE_OPTIONS})
    target_link_options(${target} PRIVATE ${COMMON_LINK_OPTIONS})
endforeach()
