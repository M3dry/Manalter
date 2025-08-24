#pragma once

#include "engine/collisions.hpp"
#include "engine/pipeline.hpp"
#include "engine/util.hpp"
#include "util_c.hpp"
#include <SDL3/SDL_gpu.h>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <memory>

#include "engine/model.hpp"

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

TAG_BY_NAME(engine::pipeline::PrimitiveType, mesh_mode_tag);
TAG_BY_NAME(uint32_t, mesh_vertex_count_tag);
TAG_BY_NAME(array_unique_ptr<glm::vec3>, mesh_positions_tag);
TAG_BY_NAME(array_unique_ptr<glm::vec3>, mesh_normals_tag);
TAG_BY_NAME(array_unique_ptr<glm::vec4>, mesh_tangents_tag);
struct mesh_texcoords_tag {};
template <> struct Named<mesh_texcoords_tag> {
    using type = std::array<array_unique_ptr<glm::vec2>, 4>;
};
TAG_BY_NAME(std::size_t, mesh_material_id);
TAG_BY_NAME(engine::collisions::AABB, mesh_bounding_box_tag);

TAG_BY_NAME(uint32_t, model_mesh_count_tag);
TAG_BY_NAME(array_unique_ptr<glm::mat4>, model_mesh_transforms_tag);
TAG_BY_NAME(array_unique_ptr<engine::model::MeshId>, model_meshes_tag);
TAG_BY_NAME(uint32_t, gpu_mesh_draw_count);
using model_gpumeshes = multi_vector<engine::model::GPUOffsets, engine::model::GPUMaterial, glm::mat4,
                                     std::array<SDL_GPUTextureSamplerBinding, 5>, gpu_mesh_draw_count, SDL_GPUBuffer*>;
TAG_BY_NAME(engine::collisions::AABB, model_bounding_box_tag);
using model_materials = std::vector<engine::model::Material>;
TAG_BY_NAME(std::size_t, model_sampler_id);
TAG_BY_NAME(std::size_t, model_source_id);
using model_textures = multi_vector<model_source_id, model_sampler_id>;
using model_samplers = std::vector<SDL_GPUSamplerCreateInfo>;
using model_sources = std::vector<engine::model::Source>;

using gpugroup_models = std::vector<engine::ModelId>;
using gpugroup_textures = std::vector<SDL_GPUTexture*>;
using gpugroup_samplers = std::vector<SDL_GPUSampler*>;
using gpugroup_buffer = SDL_GPUBuffer*;

namespace engine::archetypes {
    using Mesh =
        ecs::Archetype<model::MeshIndices, mesh_mode_tag, mesh_vertex_count_tag, mesh_positions_tag, mesh_normals_tag,
                       mesh_tangents_tag, mesh_texcoords_tag, mesh_material_id, mesh_bounding_box_tag>;
    using Model =
        ecs::Archetype<model_mesh_count_tag, model_mesh_transforms_tag, model_meshes_tag, model_gpumeshes,
                       model_bounding_box_tag, model_materials, model_textures, model_samplers, model_sources>;
    using GPUGroup = ecs::Archetype<gpugroup_models, gpugroup_textures, gpugroup_samplers, gpugroup_buffer>;
}
