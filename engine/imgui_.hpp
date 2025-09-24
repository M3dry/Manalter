#pragma once

#include <SDL3/SDL_events.h>

namespace engine::imgui {
    void init();
    void destroy();

    bool polled_event(SDL_Event* ev);
}
