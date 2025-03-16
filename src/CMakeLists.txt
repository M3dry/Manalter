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
)

set(HITBOX_DEMO_SOURCES
    hitbox_demo.cpp
    hitbox.cpp
    rayhacks.cpp
)

set(PLAYGROUND_SOURCES
    playground.cpp
)

add_library(manalter_lib STATIC ${MANALTER_LIB_SOURCES})
target_include_directories(manalter_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(manalter_lib PRIVATE raylib)
target_compile_definitions(manalter_lib PRIVATE DEBUG)

add_executable(manalter main.cpp)
target_link_libraries(manalter PRIVATE manalter_lib)

add_executable(hitbox_demo ${HITBOX_DEMO_SOURCES})
target_link_libraries(hitbox_demo PRIVATE manalter_lib)

add_executable(plaground ${PLAYGROUND_SOURCES})
target_link_libraries(plaground PRIVATE manalter_lib)
