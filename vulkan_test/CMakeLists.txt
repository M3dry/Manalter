find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
find_package(SDL3_image REQUIRED CONFIG)

set(IMGUI_SOURCES
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_demo.cpp
    imgui/backends/imgui_impl_sdl3.cpp
    imgui/backends/imgui_impl_vulkan.cpp
)

SET(SOURCES
    main.cpp

    ${IMGUI_SOURCES}
)

add_executable(vulkan_test ${SOURCES})
target_include_directories(vulkan_test PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/imgui" "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends")
target_link_libraries(vulkan_test PRIVATE glm volk vulkan vk-bootstrap SDL3::SDL3 SDL3_image::SDL3_image)
target_compile_definitions(vulkan_test PRIVATE IMGUI_IMPL_VULKAN_USE_VOLK=1)
