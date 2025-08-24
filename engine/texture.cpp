#include "engine/texture.hpp"
#include "engine/gpu.hpp"
#include "texture_c.hpp"
#include <SDL3/SDL_gpu.h>
#include <algorithm>
#include <glm/ext/vector_float4.hpp>

namespace engine::texture {
    SDL_GPUTexture* fallback = nullptr;

    void init() {
        SDL_GPUTextureCreateInfo create_info{};
        create_info.type = SDL_GPU_TEXTURETYPE_2D;
        create_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        create_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        create_info.width = 1;
        create_info.height = 1;
        create_info.num_levels = 1;
        create_info.layer_count_or_depth = 1;

        fallback = SDL_CreateGPUTexture(engine::gpu_device, &create_info);

        glm::vec<4, uint8_t> pixel{255, 255, 255, 255};
        SDL_Surface* surface = SDL_CreateSurfaceFrom(1, 1, SDL_PIXELFORMAT_RGBA8888, &pixel, 4);

        SDL_GPUCommandBuffer* cmd_buf = SDL_AcquireGPUCommandBuffer(engine::gpu_device);
        engine::gpu::transfer_to_gpu(cmd_buf, fallback, surface);
        SDL_SubmitGPUCommandBuffer(cmd_buf);

        SDL_DestroySurface(surface);
    }

    void deinit() {
        SDL_ReleaseGPUTexture(engine::gpu_device, fallback);
    }
}
