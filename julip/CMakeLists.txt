list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
find_package(Julia 1.9.0 REQUIRED)

message(STATUS Julia)

set(JULIP_SOURCES
    julip.cpp
)

add_library(julip STATIC ${JULIP_SOURCES})
target_compile_options(julip PRIVATE ${COMMON_COMPILE_OPTIONS})
target_link_options(julip PRIVATE ${COMMON_LINK_OPTIONS})
target_include_directories(julip PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> ${JULIA_INCLUDE_DIR})
target_link_libraries(julip PRIVATE ${JULIA_LIBRARY})

add_executable(julip_demo julip_demo.cpp)
target_link_libraries(julip_demo PRIVATE julip)
