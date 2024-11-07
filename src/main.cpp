#include <raylib.h>

#include "loop.hpp"

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include "loop_wrapper.hpp"
#endif

#define DEBUG

int main() {
    int width = 0, height = 0;
#ifdef PLATFORM_WEB
    emscripten_get_canvas_element_size("#canvas", &width, &height);
#endif

    InitWindow(width, height, "Aetas Magus");
    SetExitKey(KEY_NULL);
    // DisableCursor();

#ifndef PLATFORM_WEB
    width = GetScreenWidth();
    height = GetScreenHeight();
#endif

    Loop loop(width, height);

#ifdef PLATFORM_WEB
    set_loop(&loop);
    emscripten_set_main_loop(loop_wrapper, 0, 1);
#else
    while (!WindowShouldClose()) {
        loop();
    }
#endif

    CloseWindow();

    return 0;
}
