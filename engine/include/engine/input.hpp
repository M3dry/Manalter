#pragma once

#include <array>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <print>
#include <vector>

#include <SDL3/SDL_events.h>

#include <glm/ext/vector_float2.hpp>

namespace engine {
    class InputHandler {
      public:
        InputHandler();

        void update(const SDL_KeyboardEvent& ev);
        void update(const SDL_MouseMotionEvent& ev);
        void update(const SDL_Event& ev);

        bool is_held(SDL_Scancode code) const;
        bool is_held(SDL_Keycode key, SDL_Keymod* mod = nullptr) const;

        bool was_released(SDL_Scancode code) const;
        bool was_released(SDL_Keycode key, SDL_Keymod* mod = nullptr) const;

        glm::vec2 get_mouse_delta() const;

        void update_tick();
      private:
        std::bitset<SDL_SCANCODE_COUNT> down;
        std::bitset<SDL_SCANCODE_COUNT> up;
        glm::vec2 mouse_delta{0.0f};
    };

    bool poll_input(InputHandler& ih);
}
