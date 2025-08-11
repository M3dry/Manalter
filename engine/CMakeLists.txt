find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
find_package(SDL3_image REQUIRED CONFIG)
find_package(glm REQUIRED CONFIG)

set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
add_subdirectory(./tinygltf)

SET(ENGINE_SOURCES
    engine.cpp
    window.cpp
    gpu.cpp
    imgui.cpp
    events.cpp
    input.cpp
    ecs.cpp
    camera.cpp
    shader.cpp
    model.cpp
)

add_library(engine STATIC ${ENGINE_SOURCES})
target_compile_options(engine PRIVATE ${COMMON_COMPILE_OPTIONS})
target_link_options(engine PRIVATE ${COMMON_LINK_OPTIONS})
target_include_directories(engine PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} "./tinygltf")
target_link_libraries(engine PUBLIC julip ecs SDL3::SDL3 SDL3_image::SDL3_image imgui glm::glm)
target_compile_definitions(engine PUBLIC ENGINE_MODE)

add_executable(engine_demo engine_demo.cpp)
target_compile_options(engine_demo PRIVATE ${COMMON_COMPILE_OPTIONS})
target_link_options(engine_demo PRIVATE ${COMMON_LINK_OPTIONS})
target_link_libraries(engine_demo PRIVATE engine)
target_include_directories(engine_demo PRIVATE "./tinygltf")
