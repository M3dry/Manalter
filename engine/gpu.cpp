#include "engine/gpu.hpp"
#include "engine/window.hpp"
#include "gpu_c.hpp"

#include "util_c.hpp"
#include <SDL3/SDL_gpu.h>

namespace engine {
    SDL_GPUDevice* gpu_device = nullptr;
}

namespace engine::gpu {
    void init() {
        gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr);
        if (gpu_device == nullptr) {
            abort("SDL_CreateGPUDevice(): {}", SDL_GetError());
        }

        if (!SDL_ClaimWindowForGPUDevice(gpu_device, win)) {
            abort("SDL_ClaimWindowForGPUDevice(): {}", SDL_GetError());
        }
        SDL_SetGPUSwapchainParameters(gpu_device, win, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_IMMEDIATE);
    }

    void deinit() {
        SDL_WaitForGPUIdle(gpu_device);
        SDL_ReleaseWindowFromGPUDevice(gpu_device, win);
        SDL_DestroyGPUDevice(gpu_device);
    }

    void transfer_to_gpu(SDL_GPUCommandBuffer* cmd_buf, SDL_GPUBuffer* dest, void* src, uint32_t size,
                         uint32_t write_offset) {
        SDL_GPUTransferBufferCreateInfo transfer_info{
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = size,
            .props = 0,
        };
        SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(engine::gpu_device, &transfer_info);

        void* data = SDL_MapGPUTransferBuffer(engine::gpu_device, transfer_buffer, false);
        SDL_memcpy(data, src, size);
        SDL_UnmapGPUTransferBuffer(engine::gpu_device, transfer_buffer);

        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd_buf);
        SDL_GPUTransferBufferLocation location{
            .transfer_buffer = transfer_buffer,
            .offset = 0,
        };

        SDL_GPUBufferRegion region{
            .buffer = dest,
            .offset = write_offset,
            .size = size,
        };
        SDL_UploadToGPUBuffer(copy_pass, &location, &region, true);

        SDL_EndGPUCopyPass(copy_pass);
        SDL_ReleaseGPUTransferBuffer(engine::gpu_device, transfer_buffer);
    }

    void transfer_to_gpu(SDL_GPUCommandBuffer* cmd_buf, SDL_GPUTexture* dest, SDL_Surface* src) {
        SDL_GPUTransferBufferCreateInfo transfer_info{
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = static_cast<uint32_t>(src->h * src->w * 4),
            .props = 0,
        };
        SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(gpu_device, &transfer_info);

        void* data = SDL_MapGPUTransferBuffer(gpu_device, transfer_buffer, false);
        SDL_memcpy(data, src->pixels, static_cast<uint32_t>(src->h * src->w * 4));
        SDL_UnmapGPUTransferBuffer(gpu_device, transfer_buffer);

        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd_buf);
        SDL_GPUTextureTransferInfo location{
            .transfer_buffer = transfer_buffer,
            .offset = 0,
        };

        SDL_GPUTextureRegion region{
            .texture = dest,
            .w = static_cast<uint32_t>(src->w),
            .h = static_cast<uint32_t>(src->h),
            .d = 1,
        };
        SDL_UploadToGPUTexture(copy_pass, &location, &region, true);

        SDL_EndGPUCopyPass(copy_pass);
        SDL_ReleaseGPUTransferBuffer(gpu_device, transfer_buffer);
    }
}
