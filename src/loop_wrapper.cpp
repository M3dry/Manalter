#include "loop_wrapper.hpp"

Loop* loop_ptr = nullptr;

void set_loop(Loop* loop) {
    if (loop_ptr) return;
    loop_ptr = loop;
}

void loop_wrapper() {
    if (loop_ptr) {
        (*loop_ptr)();
    }
}
