#include "engine/engine.hpp"
#include "engine/event.hpp"
#include "engine/imgui.hpp"
#include "imgui/imgui.h"

int main() {
    engine::init("Test window");

    uint64_t accum = 0;
    uint64_t last_time = SDL_GetTicks();

    bool done = false;
    uint64_t frame_count = 0;

    while (!done) {
        uint64_t time = SDL_GetTicks();
        uint64_t frame_time = time - last_time;
        last_time = time;
        accum += frame_time;


        auto state = engine::event::poll();
        if (state == engine::event::Quit) {
            done = true;
        } else if (state == engine::event::Minimized) {
            SDL_Delay(10);
            continue;
        }

        while (accum >= 1000 / 60) {
            accum -= 1000 / 60;

            engine::event::update_tick();
        }

        engine::prepare_frame();
        engine::imgui::new_frame();

        ImGui::ShowDemoWindow();
        engine::imgui::prepare_draw_data();

        engine::prepare_draw();

        engine::next_frame();
    }

    engine::destroy();
    return 0;
}
