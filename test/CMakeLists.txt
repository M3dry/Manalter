find_package(Catch2 3 REQUIRED)

SET(TESTS
    enemy_spawner.t.cpp
    hitbox.t.cpp
    seria_deser.t.cpp
)

add_executable(tests ${TESTS})
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain manalter_lib)
