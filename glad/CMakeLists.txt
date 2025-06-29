add_library(glad INTERFACE)
target_include_directories(glad INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
