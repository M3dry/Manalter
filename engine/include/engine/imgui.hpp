#pragma once

namespace engine::imgui {
    bool enabled();
    void enable(bool state);

    void new_frame();
    void prepare_draw_data();
    void render();
}
