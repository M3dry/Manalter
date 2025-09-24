#include "window_c.hpp"
#include <engine/window.hpp>

#include "util_c.hpp"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>

namespace engine {
    SDL_Window* win = nullptr;
}

namespace engine::window {
    void init(const char* window_name) {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
            abort("SDL_INIT(): {}", SDL_GetError());
        }

        SDL_WindowFlags window_flags =
            SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_VULKAN | SDL_WINDOW_FULLSCREEN;
        win = SDL_CreateWindow(window_name, 1920, 1200, window_flags);
        if (win == nullptr) {
            abort("SDL_CreateWindow(): {}", SDL_GetError());
        }
        SDL_ShowWindow(win);
    }

    bool is_minimized() {
        return SDL_GetWindowFlags(win) & SDL_WINDOW_MINIMIZED;
    }

    void deinit() {
        SDL_DestroyWindow(win);
    }
}
