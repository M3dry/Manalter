find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
find_package(SDL3_image REQUIRED CONFIG)

add_executable(vulkan_test main.cpp)
target_include_directories(vulkan_test PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(vulkan_test PRIVATE glm vulkan vk-bootstrap SDL3::SDL3 SDL3_image::SDL3_image imgui)
