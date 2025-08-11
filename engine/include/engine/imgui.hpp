#pragma once

#include <SDL3/SDL_gpu.h>
namespace engine::imgui {
    bool enabled();
    void enable(bool state);

    void new_frame();
    void prepare_data(SDL_GPUCommandBuffer* cmd_buf);
    void draw(SDL_GPUCommandBuffer* cmd_buf, SDL_GPURenderPass* render_pass);
}
