#include "engine/event.hpp"
#include "engine/engine.hpp"
#include "engine/imgui.hpp"
#include "imgui_.hpp"

#include <SDL3/SDL_keyboard.h>
#include <bitset>

namespace engine::event {
    std::bitset<SDL_SCANCODE_COUNT> down;
    std::bitset<SDL_SCANCODE_COUNT> up;
    glm::vec2 mouse_delta{0.0f};

    void key_update(const SDL_KeyboardEvent& ev) {
        auto ix = static_cast<std::size_t>(ev.scancode);
        if (ev.down) {
            down.set(ix, true);
        } else if (down[ix] && !ev.down) {
            down.set(ix, false);
            up.set(ix, true);
        }
    }

    void mouse_motion_update(const SDL_MouseMotionEvent& ev) {
        mouse_delta.x += ev.xrel;
        mouse_delta.y += ev.yrel;
    }

    PollEvent poll() {
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) return Minimized;

        bool poll_imgui = imgui::enabled();
        SDL_Event ev;

        while (SDL_PollEvent(&ev)) {
            if (imgui::polled_event(&ev)) continue;

            switch (ev.type) {
                case SDL_EVENT_QUIT: return Quit;
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    if (ev.window.windowID == SDL_GetWindowID(window)) return Quit;
                    else continue;
                case SDL_EVENT_KEY_DOWN:
                case SDL_EVENT_KEY_UP: key_update(ev.key); break;
                case SDL_EVENT_MOUSE_MOTION: mouse_motion_update(ev.motion); break;
            }
        }

        return Normal;
    }

    bool is_held(SDL_Scancode code) {
        return down[static_cast<std::size_t>(code)];
    }

    bool is_held(SDL_Keycode key, SDL_Keymod* mod) {
        return down[static_cast<std::size_t>(SDL_GetScancodeFromKey(key, mod))];
    }

    bool was_released(SDL_Scancode code) {
        return up[static_cast<std::size_t>(code)];
    }

    bool was_released(SDL_Keycode key, SDL_Keymod* mod) {
        return up[static_cast<std::size_t>(SDL_GetScancodeFromKey(key, mod))];
    }

    glm::vec2 get_mouse_delta() {
        return mouse_delta;
    }

    void update_tick() {
        mouse_delta = glm::vec2{0.0f};
        up.reset();
    }
}
