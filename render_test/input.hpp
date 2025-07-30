#pragma once

#include "SDL3/SDL_keyboard.h"
#include <array>
#include <bitset>
#include <cassert>
#include <cstdint>

#include <SDL3/SDL_events.h>
#include <print>
#include <vector>

class InputHandler {
  public:
    std::bitset<SDL_SCANCODE_COUNT> down;
    std::bitset<SDL_SCANCODE_COUNT> up;

    InputHandler() {};

    void update(SDL_KeyboardEvent ev) {
        auto ix = static_cast<std::size_t>(ev.scancode);
        if (ev.down) {
            down.set(ix, true);
        } else if (down[ix] && !ev.down) {
            down.set(ix, false);
            up.set(ix, true);
        } else {
            // ignore, can happen with WM keybinds
        }
    }

    bool is_held(SDL_Scancode code) {
        return down[static_cast<std::size_t>(code)];
    }

    bool is_held(SDL_Keycode code, SDL_Keymod* mod = nullptr) {
        return down[SDL_GetScancodeFromKey(code, mod)];
    }
};
