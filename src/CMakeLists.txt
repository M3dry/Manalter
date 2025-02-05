set(MANALTER_SOURCES
    main.cpp
    assets.cpp
    hitbox.cpp
    loop.cpp
    loop_wrapper.cpp
    rayhacks.cpp
    spell.cpp
    spell_caster.cpp
    utility.cpp
    ringbuffer.cpp
    enemies.cpp
    item_drops.cpp
)

set(HITBOX_DEMO_SOURCES
    hitbox_demo.cpp
    hitbox.cpp
    rayhacks.cpp
)

set(PLAYGROUND_SOURCES
    playground.cpp
)

add_executable(manalter ${MANALTER_SOURCES})
target_link_libraries(manalter PRIVATE raylib)
target_include_directories(manalter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(manalter PRIVATE DEBUG)

add_executable(hitbox_demo ${HITBOX_DEMO_SOURCES})
target_link_libraries(hitbox_demo PRIVATE raylib)
target_include_directories(hitbox_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(plaground ${PLAYGROUND_SOURCES})
target_link_libraries(plaground PRIVATE raylib)
target_include_directories(plaground PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
