set(APP_SOURCES
    main.cpp
    assets.cpp
    hitbox.cpp
    loop.cpp
    loop_wrapper.cpp
    rayhacks.cpp
    spell.cpp
)

add_executable(manalter ${APP_SOURCES})

target_link_libraries(manalter PRIVATE raylib)

target_include_directories(manalter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
