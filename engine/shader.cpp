#include "engine/shader.hpp"
#include "engine/gpu.hpp"
#include "util_c.hpp"

namespace engine::shader {
    SDL_GPUShader* load_shader(const std::span<uint8_t>& code, SDL_GPUShaderStage stage, uint32_t samplers,
                               uint32_t textures, uint32_t storage_buffers, uint32_t uniforms) {
        SDL_GPUShaderCreateInfo info{
            .code_size = code.size(),
            .code = code.data(),
            .entrypoint = "main",
            .format = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage = stage,
            .num_samplers = samplers,
            .num_storage_textures = textures,
            .num_storage_buffers = storage_buffers,
            .num_uniform_buffers = uniforms,
            .props = 0,
        };

        return SDL_CreateGPUShader(gpu_device, &info);
    }

    SDL_GPUShader* load_shader_from_file(const char* file, SDL_GPUShaderStage stage, uint32_t samplers,
                                         uint32_t textures, uint32_t storage_buffers, uint32_t uniforms) {
        std::size_t code_size;
        void* code = SDL_LoadFile(file, &code_size);
        SDL_GPUShader* shader =
            load_shader({(uint8_t*)code, code_size}, stage, samplers, textures, storage_buffers, uniforms);
        if (shader == nullptr) {
            abort("load_shader_from-file: {}; file size: {}", SDL_GetError(), code_size);
        }
        SDL_free(code);

        return shader;
    }

    SDL_GPUShader* Make::load(const std::span<uint8_t>& code) {
        return load_shader(code, _stage, _samplers, _textures, _storage_buffers, _uniforms);
    }

    SDL_GPUShader* Make::load_from_file(const char* file) {
        return load_shader_from_file(file, _stage, _samplers, _textures, _storage_buffers, _uniforms);
    }
}
