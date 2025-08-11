#pragma once

#include "SDL3/SDL_gpu.h"
#include <algorithm>
#include <cstring>
#include <glm/ext/vector_float3.hpp>
#include <memory>

template <typename T>
void transfer_to_gpu(SDL_GPUDevice* gpu, SDL_GPUCommandBuffer* cmd_buf, SDL_GPUBuffer* dest, T* src, std::size_t size,
                     uint32_t write_offset_index) {
    SDL_GPUTransferBufferCreateInfo transfer_info{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = static_cast<uint32_t>(sizeof(T) * size),
        .props = 0,
    };
    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(gpu, &transfer_info);

    T* data = (T*)SDL_MapGPUTransferBuffer(gpu, transfer_buffer, false);
    std::memcpy(data, src, sizeof(T) * size);
    SDL_UnmapGPUTransferBuffer(gpu, transfer_buffer);

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd_buf);
    SDL_GPUTransferBufferLocation location{
        .transfer_buffer = transfer_buffer,
        .offset = 0,
    };

    SDL_GPUBufferRegion region{
        .buffer = dest,
        .offset = static_cast<uint32_t>(sizeof(T) * write_offset_index),
        .size = static_cast<uint32_t>(sizeof(T) * size),
    };
    SDL_UploadToGPUBuffer(copy_pass, &location, &region, true);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_ReleaseGPUTransferBuffer(gpu, transfer_buffer);
}

inline void transfer_to_gpu(SDL_GPUDevice* gpu, SDL_GPUCommandBuffer* cmd_buf, SDL_GPUTexture* dest, SDL_Surface* src) {
    SDL_GPUTransferBufferCreateInfo transfer_info{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = static_cast<uint32_t>(src->h * src->w * 4),
        .props = 0,
    };
    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(gpu, &transfer_info);

    void* data = SDL_MapGPUTransferBuffer(gpu, transfer_buffer, false);
    std::memcpy(data, src->pixels, static_cast<uint32_t>(src->h * src->w * 4));
    SDL_UnmapGPUTransferBuffer(gpu, transfer_buffer);

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
    SDL_ReleaseGPUTransferBuffer(gpu, transfer_buffer);
}

struct GPUMesh {
    SDL_GPUBuffer* index_buffer;
    SDL_GPUBuffer* vertex_buffer;

    GPUMesh() : index_buffer(nullptr), vertex_buffer(nullptr) {};
    GPUMesh(SDL_GPUBuffer* index_buffer, SDL_GPUBuffer* vertex_buffer) : index_buffer(index_buffer), vertex_buffer(vertex_buffer) {};

    GPUMesh(const GPUMesh&) = delete;
    GPUMesh& operator=(const GPUMesh&) = delete;

    GPUMesh(GPUMesh&& gpu_mesh) noexcept : index_buffer(gpu_mesh.index_buffer), vertex_buffer(gpu_mesh.vertex_buffer) {
        gpu_mesh.index_buffer = nullptr;
        gpu_mesh.vertex_buffer = nullptr;
    };
    GPUMesh& operator=(GPUMesh&& gpu_mesh) noexcept {
        index_buffer = gpu_mesh.index_buffer;
        vertex_buffer = gpu_mesh.vertex_buffer;

        gpu_mesh.index_buffer = nullptr;
        gpu_mesh.vertex_buffer = nullptr;

        return *this;
    };

    void release(SDL_GPUDevice* gpu) {
        if (index_buffer != nullptr) SDL_ReleaseGPUBuffer(gpu, index_buffer);
        if (vertex_buffer != nullptr) SDL_ReleaseGPUBuffer(gpu, vertex_buffer);
    }

    // TODO: need GPUDevice to release buffers, for now call `.release`
    ~GPUMesh() {
    }
};

struct Mesh {
    uint32_t* indexes;
    std::size_t index_count;
    glm::vec3* vertices;
    std::size_t vertex_count;

    GPUMesh create_on_gpu(SDL_GPUDevice* gpu, SDL_GPUCommandBuffer* cmd_buf) {
        SDL_GPUBufferCreateInfo index_buffer_info{
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = static_cast<uint32_t>(index_count * sizeof(uint32_t)),
            .props = 0,
        };
        SDL_GPUBufferCreateInfo vertex_buffer_info{
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = static_cast<uint32_t>(vertex_count * sizeof(glm::vec3)),
            .props = 0,
        };

        SDL_GPUBuffer* index_buffer = SDL_CreateGPUBuffer(gpu, &index_buffer_info);
        SDL_GPUBuffer* vertex_buffer = SDL_CreateGPUBuffer(gpu, &vertex_buffer_info);

        transfer_to_gpu(gpu, cmd_buf, index_buffer, indexes, index_count, 0);
        transfer_to_gpu(gpu, cmd_buf, vertex_buffer, vertices, vertex_count, 0);

        return GPUMesh{index_buffer, vertex_buffer};
    }
};
