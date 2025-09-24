#pragma once

#include "engine/shader.hpp"
#include "multiarray.hpp"

#include <SDL3/SDL_gpu.h>
#include <cstdint>

namespace engine::shader {
    struct ShaderDesc {
        uint8_t code_hash[4];
        uint8_t stage : 1;
        uint8_t uniforms : 8;
        uint8_t storage_buffers : 8;
        uint8_t samplers : 8;
        uint8_t storage_textures : 7;

        ShaderDesc(SDL_GPUShaderCreateInfo desc);

        bool operator==(const ShaderDesc& other);
    };

    extern multi_vector<ShaderDesc, SDL_GPUShader*> shaders;

    ShaderId create_shader(const SDL_GPUShaderCreateInfo& info);
    std::optional<ShaderId> shader_exists(const ShaderDesc& desc);
}
