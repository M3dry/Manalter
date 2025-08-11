#pragma once

#include <SDL3/SDL_gpu.h>
#include <cstdint>
#include <span>
namespace engine::shader {
    enum Stage {
        Vertex = SDL_GPU_SHADERSTAGE_VERTEX,
        Fragment = SDL_GPU_SHADERSTAGE_FRAGMENT,
    };

    class Make {
      public:
        Make() {};

        Make& uniforms(uint32_t count) {
            _uniforms = count;
            return *this;
        }

        Make& textures(uint32_t count) {
            _textures = count;
            return *this;
        }

        Make& storage_buffers(uint32_t count) {
            _storage_buffers = count;
            return *this;
        }

        Make& samplers(uint32_t count) {
            _samplers = count;
            return *this;
        }

        Make& stage(Stage stage) {
            _stage = static_cast<SDL_GPUShaderStage>(stage);
            return *this;
        }

        SDL_GPUShader* load(const std::span<uint8_t>& code);
        SDL_GPUShader* load_from_file(const char* file);

      private:
        uint32_t _uniforms = 0;
        uint32_t _textures = 0;
        uint32_t _storage_buffers = 0;
        uint32_t _samplers = 0;
        SDL_GPUShaderStage _stage;
    };
}
