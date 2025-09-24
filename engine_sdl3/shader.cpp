#include "engine/shader.hpp"
#include "engine/gpu.hpp"
#include "util_c.hpp"
#include "shader_c.hpp"

#include <SDL3/SDL_gpu.h>
#include <cstring>
#include <xxh3.h>

namespace engine::shader {
    multi_vector<ShaderDesc, SDL_GPUShader*> shaders{};

    ShaderId create_shader(const SDL_GPUShaderCreateInfo& info) {
        ShaderDesc desc{info};
        if (auto exists = shader_exists(desc); exists) {
            return *exists;
        }

        auto [out_desc, out_shader] = shaders.unsafe_push();
        *out_desc = desc;
        *out_shader = SDL_CreateGPUShader(gpu_device, &info);

        return ShaderId{(uint16_t)(shaders.size() - 1), *out_shader};
    }

    std::optional<ShaderId> shader_exists(const ShaderDesc& desc) {
        auto [descs, ptrs] = shaders.get_span<ShaderDesc, SDL_GPUShader*>();
        for (std::size_t i = 0; i < descs.size(); i++) {
            if (descs[i] == desc) return ShaderId{(uint16_t)i, ptrs[i]};
        }

        return std::nullopt;
    }

    ShaderId load_shader(const std::span<uint8_t>& code, SDL_GPUShaderStage stage, uint32_t samplers,
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

        return create_shader(info);
    }

    ShaderId load_shader_from_file(const char* file, SDL_GPUShaderStage stage, uint32_t samplers,
                                         uint32_t textures, uint32_t storage_buffers, uint32_t uniforms) {
        std::size_t code_size;
        void* code = SDL_LoadFile(file, &code_size);
        ShaderId shader = load_shader({(uint8_t*)code, code_size}, stage, samplers, textures, storage_buffers, uniforms);
        if ((SDL_GPUShader*)shader == nullptr) {
            abort("load_shader_from-file: {}; file size: {}", SDL_GetError(), code_size);
        }
        SDL_free(code);

        return shader;
    }

    ShaderId Make::load(const std::span<uint8_t>& code) {
        return load_shader(code, _stage, _samplers, _textures, _storage_buffers, _uniforms);
    }

    ShaderId Make::load_from_file(const char* file) {
        return load_shader_from_file(file, _stage, _samplers, _textures, _storage_buffers, _uniforms);
    }

    ShaderDesc::ShaderDesc(SDL_GPUShaderCreateInfo desc) {
        stage = desc.stage;
        uniforms = (uint8_t)desc.num_uniform_buffers;
        storage_buffers = (uint8_t)desc.num_storage_buffers;
        samplers = (uint8_t)desc.num_samplers;
        storage_textures = (uint8_t)desc.num_storage_textures;

        auto hash = XXH3_64bits(desc.code, desc.code_size);
        std::memcpy(&code_hash, &hash, sizeof(uint64_t));
    }

    bool ShaderDesc::operator==(const ShaderDesc& other) {
        return std::memcmp(this, &other, sizeof(ShaderDesc)) == 0;
    }
}
