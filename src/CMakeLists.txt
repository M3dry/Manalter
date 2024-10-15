set(APP_SOURCES
    # main.cpp
    test.cpp
)

add_executable(aetas_magus ${APP_SOURCES})

target_link_libraries(aetas_magus PRIVATE raylib)

target_include_directories(aetas_magus PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
