set(ECS_SOURCES
    ecs.cpp
)

add_library(ecs STATIC ${ECS_SOURCES})
target_compile_options(ecs PRIVATE ${COMMON_COMPILE_OPTIONS})
target_link_options(ecs PRIVATE ${COMMON_LINK_OPTIONS})
target_include_directories(ecs PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(ecs PUBLIC julip)

add_executable(ecs_demo ecs_demo.cpp)
target_compile_options(ecs_demo PRIVATE ${COMMON_COMPILE_OPTIONS})
target_link_options(ecs_demo PRIVATE ${COMMON_LINK_OPTIONS})
target_link_libraries(ecs_demo PRIVATE ecs)

find_package(Catch2 3 REQUIRED)
add_executable(test ecs.test.cpp)
target_link_libraries(test PRIVATE Catch2::Catch2WithMain ecs)
