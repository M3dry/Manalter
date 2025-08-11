#include "engine/events.hpp"

#include <imgui_impl_sdl3.h>

namespace engine::imgui {
    bool poll_event(const SDL_Event* ev) {
        if (!engine::imgui::enabled()) {
            return false;
        } else {
            ImGuiIO& io = ImGui::GetIO();
            ImGui_ImplSDL3_ProcessEvent(ev);

            bool is_key_event = ev->type == SDL_EVENT_KEY_DOWN || ev->type == SDL_EVENT_KEY_UP;
            bool is_mouse_event = ev->type == SDL_EVENT_MOUSE_BUTTON_DOWN || ev->type == SDL_EVENT_MOUSE_BUTTON_UP ||
                                  ev->type == SDL_EVENT_MOUSE_WHEEL || ev->type == SDL_EVENT_MOUSE_MOTION;
            return (io.WantCaptureKeyboard && is_key_event) || (io.WantCaptureMouse && is_mouse_event);
        }
    }
}
