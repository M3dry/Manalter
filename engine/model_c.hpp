#pragma once

#include "engine/util.hpp"
#include "util_c.hpp"
#include <SDL3/SDL_gpu.h>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <memory>

namespace engine::model {
    MAKE_WRAPPED_ID(MeshId);

    struct MeshIndices {
        SDL_GPUIndexElementSize stride;
        size_t count;
        // nullptr implies no index draw
        array_unique_ptr<std::byte> indices;

        MeshIndices() {};

        MeshIndices(MeshIndices&&) noexcept = default;
        MeshIndices& operator=(MeshIndices&&) noexcept = default;
    };

    enum struct TexcoordMap {
        TEXCOORD_0 = 0,
        TEXCOORD_1 = 1,
        TEXCOORD_2 = 2,
        TEXCOORD_3 = 3,
    };

    enum struct TexcoordKey {
        BaseColor = 0,
        MetallicRoughnes = 1,
        Normal = 2,
        Occlusion = 3,
        Emissive = 4,
    };

    struct GPUOffsets {
        enum Attribute {
            Indices = 1 << 0,
            Position = 1 << 1,
            Normal = 1 << 2,
            Tangent = 1 << 3,
            BaseColorCoord = 1 << 4,
            MetallicRoughnessCoord = 1 << 5,
            NormalCoord = 1 << 6,
            OcclusionCoord = 1 << 7,
            EmissiveCoord = 1 << 8,
        };

        uint32_t start_offset;
        uint32_t indices_offset;
        uint32_t position_offset;
        uint32_t normal_offset;
        uint32_t tangent_offset;
        uint32_t base_color_coord_offset;
        uint32_t metallic_roughness_coord_offset;
        uint32_t normal_coord_offset;
        uint32_t occlusion_coord_offset;
        uint32_t emissive_coord_offset;
        uint32_t stride;
        uint32_t attribute_mask;
    };

    struct Material {
        glm::vec4 base_color;
        std::size_t base_color_texture;
        float metallic_factor;
        float roughness_factor;
        std::size_t metallic_roughness_texture;
        float normal_factor;
        std::size_t normal_texture;
        float occlusion_factor;
        std::size_t occlusion_texture;
        glm::vec3 emissive_factor;
        std::size_t emissive_texture;
        std::array<TexcoordMap, 5> texcoord_mapping;
    };

    struct Source {
        array_unique_ptr<std::byte> data;
        uint64_t size;
        uint16_t width;
        uint16_t height;
        SDL_GPUTextureFormat format;
    };

    struct GPUMaterial {
        glm::vec4 base_color;
        glm::vec4 metallic_roughness_normal_occlusion_factors;
        glm::vec3 emissive_factor;
    };

    inline constexpr SDL_GPUSamplerCreateInfo default_sampler{
        .min_filter = SDL_GPU_FILTER_LINEAR,
        .mag_filter = SDL_GPU_FILTER_LINEAR,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .enable_anisotropy = false,
        .enable_compare = false,
    };

    inline constexpr Material default_material{
        .base_color = glm::vec4{1.0f},
        .base_color_texture = (std::size_t)-1,
        .metallic_factor = 1.0f,
        .roughness_factor = 1.0f,
        .metallic_roughness_texture = (std::size_t)-1,
        .normal_factor = 1.0f,
        .normal_texture = (std::size_t)-1,
        .occlusion_factor = 1.0f,
        .occlusion_texture = (std::size_t)-1,
        .emissive_factor = glm::vec3{0.0f},
        .emissive_texture = (std::size_t)-1,
        .texcoord_mapping =
            {
                TexcoordMap::TEXCOORD_0,
                TexcoordMap::TEXCOORD_0,
                TexcoordMap::TEXCOORD_0,
                TexcoordMap::TEXCOORD_0,
                TexcoordMap::TEXCOORD_0,
            },
    };
}
