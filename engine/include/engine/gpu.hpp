#pragma once

#include <SDL3/SDL_gpu.h>

#include <span>

namespace engine {
    extern SDL_GPUDevice* gpu_device;
}

namespace engine::gpu {
    void transfer_to_gpu(SDL_GPUCommandBuffer* cmd_buf, SDL_GPUBuffer* dest, void* src, uint32_t size,
                         uint32_t write_offset);
    void transfer_to_gpu(SDL_GPUCommandBuffer* cmd_buf, SDL_GPUTexture* dest, SDL_Surface* src);
}
