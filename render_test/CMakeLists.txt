find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
find_package(SDL3_image REQUIRED CONFIG)
find_package(glm REQUIRED CONFIG)

add_executable(render_test main.cpp)
target_compile_options(render_test PRIVATE ${COMMON_COMPILE_OPTIONS})
target_link_options(render_test PRIVATE ${COMMON_LINK_OPTIONS})
target_include_directories(render_test PRIVATE SDL3::Headers)
target_link_libraries(render_test PRIVATE SDL3::SDL3 SDL3_image::SDL3_image imgui glm::glm ecs)
