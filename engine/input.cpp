#include "engine/input.hpp"
#include "engine/events.hpp"
#include "engine/window.hpp"

namespace engine {
    InputHandler::InputHandler() {};

    void InputHandler::update(const SDL_KeyboardEvent& ev) {
        auto ix = static_cast<std::size_t>(ev.scancode);
        if (ev.down) {
            down.set(ix, true);
        } else if (down[ix] && !ev.down) {
            down.set(ix, false);
            up.set(ix, true);
        }
    }

    void InputHandler::update(const SDL_MouseMotionEvent& ev) {
        mouse_delta.x += ev.xrel;
        mouse_delta.y += ev.yrel;
    }

    void InputHandler::update(const SDL_Event& ev) {
        if (ev.type == SDL_EVENT_KEY_DOWN || ev.type == SDL_EVENT_KEY_UP) {
            update(ev.key);
        } else if (ev.type == SDL_EVENT_MOUSE_MOTION) {
            update(ev.motion);
        }
    }

    bool InputHandler::is_held(SDL_Scancode code) const {
        return down[static_cast<std::size_t>(code)];
    }
    bool InputHandler::is_held(SDL_Keycode key, SDL_Keymod* mod) const {
        return down[SDL_GetScancodeFromKey(key, mod)];
    }

    bool InputHandler::was_released(SDL_Scancode code) const {
        return up[static_cast<std::size_t>(code)];
    }

    bool InputHandler::was_released(SDL_Keycode key, SDL_Keymod* mod) const {
        return up[SDL_GetScancodeFromKey(key, mod)];
    }

    glm::vec2 InputHandler::get_mouse_delta() const {
        return mouse_delta;
    }

    void InputHandler::update_tick() {
        mouse_delta = glm::vec2{0.0f};
        up.reset();
    }

    bool poll_input(InputHandler& ih) {
        bool should_quit = false;

        poll_events([&](SDL_Event& ev) {
            if (ev.type == SDL_EVENT_QUIT ||
                (ev.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && ev.window.windowID == SDL_GetWindowID(win))) {
                should_quit = true;
                return;
            }

            ih.update(ev);
        });

        return should_quit;
    }
}
