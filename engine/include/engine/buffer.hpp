#pragma once

#include "engine/gpu.hpp"
#include <SDL3/SDL_gpu.h>
#include <cstddef>

namespace engine {
    namespace buffer {
        enum BufferUsage {
            Vertex =  SDL_GPU_BUFFERUSAGE_VERTEX,
            Index =  SDL_GPU_BUFFERUSAGE_INDEX,
            Indirect =  SDL_GPU_BUFFERUSAGE_INDIRECT,
            GraphicsStorageRead =  SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
            ComputeStorageRead =  SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ,
            ComputeStorageWrite =  SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE,
        };
    }

    class Buffer {
      public:
        Buffer() {};
        Buffer(uint32_t size, buffer::BufferUsage usage_flags) : _size(size) {
            SDL_GPUBufferCreateInfo create_info{
                .usage = usage_flags,
                .size = size,
                .props = 0,
            };
            buffer = SDL_CreateGPUBuffer(engine::gpu_device, &create_info);
        }

        Buffer(Buffer&& buf) noexcept : buffer(buf.buffer), _size(buf._size) {
            buf.buffer = nullptr;
            buf._size = 0;
        }
        Buffer& operator=(Buffer&& buf) noexcept {
            if (this == &buf) return *this;

            buffer = buf.buffer;
            _size = buf._size;
            buf.buffer = nullptr;
            buf._size = 0;

            return *this;
        }

        ~Buffer() {
            if (buffer != nullptr) {
                SDL_ReleaseGPUBuffer(engine::gpu_device, buffer);
            }
        }

        uint32_t size() const {
            return _size;
        }

        void name(const char* name) {
            SDL_SetGPUBufferName(engine::gpu_device, buffer, name);
        }

        SDL_GPUBuffer* release() {
            auto buf = buffer;
            buffer = nullptr;
            return buf;
        }

        operator SDL_GPUBuffer*() {
            return buffer;
        }
      private:
        SDL_GPUBuffer* buffer = nullptr;
        uint32_t _size = 0;
    };
}
