add_executable(mote_demo mote_demo.cpp)
target_link_libraries(mote_demo PRIVATE ecs raylib)
target_compile_options(mote_demo PRIVATE ${COMMON_COMPILE_OPTIONS})
target_link_options(mote_demo PRIVATE ${COMMON_LINK_OPTIONS} -g)
