set(MANALTER_SOURCES
    main.cpp
    assets.cpp
    hitbox.cpp
    loop.cpp
    loop_wrapper.cpp
    rayhacks.cpp
    spell.cpp
)

set(HITBOX_DEMO_SOURCES
    hitbox_demo.cpp
    hitbox.cpp
    rayhacks.cpp
)

add_executable(manalter ${MANALTER_SOURCES})
target_link_libraries(manalter PRIVATE raylib)
target_include_directories(manalter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(hitbox_demo ${HITBOX_DEMO_SOURCES})
target_link_libraries(hitbox_demo PRIVATE raylib)
target_include_directories(hitbox_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
