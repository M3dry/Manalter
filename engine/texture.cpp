#include "engine/texture.hpp"
#include "engine/gpu.hpp"
#include "texture_c.hpp"
#include <SDL3/SDL_gpu.h>
#include <algorithm>
#include <glm/ext/vector_float4.hpp>

namespace engine::texture {
    SDL_GPUTexture* fallback = nullptr;

    void init() {
        // SDL_GPUTextureCreateInfo create_info;
        // create_info.type = SDL_GPU_TEXTURETYPE_2D;
        // create_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        // create_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        // create_info.width = 1;
        // create_info.height = 1;
        // create_info.num_levels = 1;
        //
        // fallback = SDL_CreateGPUTexture(engine::gpu_device, &create_info);
        //
        // SDL_GPUCommandBuffer* cmd_buf = SDL_AcquireGPUCommandBuffer(engine::gpu_device);
        // SDL_GPUTransferBufferCreateInfo transfer_info{
        //     .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        //     .size = 4,
        //     .props = 0,
        // };
        // SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(engine::gpu_device, &transfer_info);
        //
        // void* data = SDL_MapGPUTransferBuffer(engine::gpu_device, transfer_buffer, false);
        // *(glm::vec<4, uint8_t>*)data = glm::vec<4, uint8_t>{255, 255, 255, 255};
        // SDL_UnmapGPUTransferBuffer(engine::gpu_device, transfer_buffer);
        //
        // SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd_buf);
        // SDL_GPUTextureTransferInfo location{
        //     .transfer_buffer = transfer_buffer,
        //     .offset = 0,
        // };
        //
        // SDL_GPUTextureRegion region{
        //     .texture = fallback,
        //     .mip_level = 1,
        //     .layer = 0,
        //     .x = 0,
        //     .y = 0,
        //     .z = 0,
        //     .w = 1,
        //     .h = 1,
        //     .d = 1,
        // };
        // SDL_UploadToGPUTexture(copy_pass, &location, &region, false);
        //
        // SDL_EndGPUCopyPass(copy_pass);
        // SDL_SubmitGPUCommandBuffer(cmd_buf);
        // SDL_ReleaseGPUTransferBuffer(engine::gpu_device, transfer_buffer);
    }

    void deinit() {
        // SDL_ReleaseGPUTexture(engine::gpu_device, fallback);
    }
}
