#pragma once

#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_scancode.h"
#include <array>
#include <bitset>
#include <cassert>
#include <cstdint>

#include <SDL3/SDL_events.h>
#include <glm/ext/vector_float2.hpp>
#include <print>
#include <vector>

class InputHandler {
  public:
    std::bitset<SDL_SCANCODE_COUNT> down;
    std::bitset<SDL_SCANCODE_COUNT> up;

    glm::vec2 mouse_delta{0.0f};

    InputHandler() {};

    void update(SDL_KeyboardEvent ev) {
        auto ix = static_cast<std::size_t>(ev.scancode);
        if (ev.down) {
            down.set(ix, true);
        } else if (down[ix] && !ev.down) {
            down.set(ix, false);
            up.set(ix, true);
        }
    }

    void update(SDL_MouseMotionEvent ev) {
        mouse_delta.x += ev.xrel;
        mouse_delta.y += ev.yrel;
    }

    bool is_held(SDL_Scancode code) {
        return down[static_cast<std::size_t>(code)];
    }

    bool is_held(SDL_Keycode key, SDL_Keymod* mod = nullptr) {
        return down[SDL_GetScancodeFromKey(key, mod)];
    }

    bool was_released(SDL_Scancode code) const {
        return up[static_cast<std::size_t>(code)];
    }

    bool was_released(SDL_Keycode key, SDL_Keymod* mod = nullptr) const {
        return up[SDL_GetScancodeFromKey(key, mod)];
    }

    void update_tick() {
        mouse_delta = glm::vec2{0.0f};
        up.reset();
    }
};
