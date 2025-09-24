#pragma once

#include <SDL3/SDL_events.h>
#include <glm/ext/vector_float2.hpp>

namespace engine::event {
    enum PollEvent {
        Normal,
        Minimized,
        Quit,
    };

    PollEvent poll();

    bool is_held(SDL_Scancode code);
    bool is_held(SDL_Keycode key, SDL_Keymod* mod);

    bool was_released(SDL_Scancode code);
    bool was_released(SDL_Keycode key, SDL_Keymod* mod);

    glm::vec2 get_mouse_delta();

    void update_tick();
}
