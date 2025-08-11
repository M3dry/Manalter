#pragma once

#include <SDL3/SDL_video.h>

namespace engine {
    extern SDL_Window* win;

    namespace window {
        bool is_minimized();
    }
}
