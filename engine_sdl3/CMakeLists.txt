find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
find_package(SDL3_image REQUIRED CONFIG)
find_package(glm REQUIRED CONFIG)
find_package(xxHash 0.8 CONFIG REQUIRED)

if (NOT TARGET tinygltf)
    set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
    set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
    add_subdirectory(./tinygltf)
endif()

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
    pipeline.cpp
    model.cpp
    texture.cpp
    MikkTSpace/mikktspace.c
)

add_library(engine_sdl3 STATIC ${ENGINE_SOURCES})
target_compile_options(engine_sdl3 PRIVATE ${COMMON_COMPILE_OPTIONS})
target_link_options(engine_sdl3 PRIVATE ${COMMON_LINK_OPTIONS})
target_include_directories(engine_sdl3 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(engine_sdl3 PUBLIC julip ecs SDL3::SDL3 SDL3_image::SDL3_image imgui glm::glm xxHash::xxhash)
target_compile_definitions(engine_sdl3 PUBLIC ENGINE_MODE)

add_executable(engine_sdl3_demo engine_demo.cpp)
target_compile_options(engine_sdl3_demo PRIVATE ${COMMON_COMPILE_OPTIONS})
target_link_options(engine_sdl3_demo PRIVATE ${COMMON_LINK_OPTIONS})
target_link_libraries(engine_sdl3_demo PRIVATE engine_sdl3)
target_include_directories(engine_sdl3_demo PRIVATE "./tinygltf")
