#pragma once

#include <SDL3/SDL_events.h>

#include "engine/imgui.hpp"

namespace engine {
    namespace imgui {
        bool poll_event(const SDL_Event* ev);
    }

    template <typename F> void poll_events(F&& f) {
        bool process_imgui = imgui::enabled();
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (!imgui::poll_event(&ev)) {
                f(ev);
            }
        }
    }
}
