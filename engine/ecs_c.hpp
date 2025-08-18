#pragma once

#include "ecs.hpp"
#include "engine/buffer.hpp"
#include "engine/camera.hpp"
#include "engine/collisions.hpp"
#include "engine/config.hpp"
#include "engine/model.hpp"
#include "engine/pipeline.hpp"
#include "model_c.hpp"
#include "multiarray.hpp"
#include "typeset.hpp"
#include "util_c.hpp"

#include <glm/ext/matrix_float4x4.hpp>

TAG_BY_NAME(glm::vec3, cam_position_tag);
TAG_BY_NAME(glm::quat, cam_orientation_tag);
TAG_BY_NAME(glm::mat4, cam_proj_tag);
TAG_BY_NAME(glm::mat4, cam_view_tag);
TAG_BY_NAME(glm::mat4, cam_view_proj_tag);

// debug cam data
TAG_BY_NAME(std::string, camera_name_tag);
TAG_BY_NAME(float, yaw_tag);
TAG_BY_NAME(float, pitch_tag);
TAG_BY_NAME(glm::vec3, focus_point_tag);

TAG_BY_NAME(float, material_base_color_factor);

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
    template <typename... Ts>
    using Cam = typeset::set_union_t<
        type_set<Ts..., cam_position_tag, cam_orientation_tag, cam_view_tag, cam_proj_tag, cam_view_proj_tag>,
        std::conditional_t<config::debug_mode, type_set<camera_name_tag, yaw_tag, pitch_tag, focus_point_tag>,
                           type_set<>>,
        ecs::Archetype>;
    using PerspCam = Cam<camera::Perspective>;
    using OrthoCam = Cam<camera::Orthographic>;

    using Material = ecs::Archetype<material_base_color_factor>;
    using Mesh =
        ecs::Archetype<model::MeshIndices, mesh_mode_tag, mesh_vertex_count_tag, mesh_positions_tag, mesh_normals_tag,
                       mesh_tangents_tag, mesh_texcoords_tag, mesh_material_id, mesh_bounding_box_tag>;
    using Model =
        ecs::Archetype<model_mesh_count_tag, model_mesh_transforms_tag, model_meshes_tag, model_gpumeshes,
                       model_bounding_box_tag, model_materials, model_textures, model_samplers, model_sources>;
    using GPUGroup = ecs::Archetype<gpugroup_models, gpugroup_textures,
                                    gpugroup_samplers, gpugroup_buffer>;
}

namespace engine {
    using ECS = ecs::build<archetypes::PerspCam, archetypes::OrthoCam, archetypes::Model, archetypes::Mesh,
                           archetypes::GPUGroup>;
    extern ECS ecs;
}
