#pragma once

#include "ecs.hpp"
#include "engine/buffer.hpp"
#include "engine/camera.hpp"
#include "engine/collisions.hpp"
#include "engine/config.hpp"
#include "engine/model.hpp"
#include "engine/pipeline.hpp"
#include "model_c.hpp"
#include "util_c.hpp"
#include "typeset.hpp"

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

TAG_BY_NAME(glm::mat4, mesh_transform);
TAG_BY_NAME(engine::pipeline::PrimitiveType, mesh_mode_tag);
TAG_BY_NAME(uint32_t, mesh_vertex_count_tag);
TAG_BY_NAME(array_unique_ptr<glm::vec3>, mesh_positions_tag);
TAG_BY_NAME(array_unique_ptr<glm::vec3>, mesh_normals_tag);
TAG_BY_NAME(engine::collisions::AABB, mesh_bounding_box_tag);

TAG_BY_NAME(uint32_t, model_mesh_count_tag);
TAG_BY_NAME(array_unique_ptr<engine::MeshId>, model_meshes_tag);
TAG_BY_NAME(array_unique_ptr<engine::GPUMeshId>, model_gpu_meshes_tag);
TAG_BY_NAME(engine::collisions::AABB, model_bounding_box_tag);

namespace engine::archetypes {
    template <typename... Ts>
    using Cam = typeset::set_union_t<
        type_set<Ts..., cam_position_tag, cam_orientation_tag, cam_view_tag, cam_proj_tag, cam_view_proj_tag>,
        std::conditional_t<config::debug_mode, type_set<camera_name_tag, yaw_tag, pitch_tag, focus_point_tag>,
                           type_set<>>,
        ecs::Archetype>;
    using PerspCam = Cam<camera::Perspective>;
    using OrthoCam = Cam<camera::Orthographic>;

    using Model = ecs::Archetype<model_mesh_count_tag, model_meshes_tag, model_gpu_meshes_tag, model_bounding_box_tag>;
    using Mesh = ecs::Archetype<mesh_transform, model::MeshIndices, mesh_mode_tag, mesh_vertex_count_tag, mesh_positions_tag, mesh_normals_tag, mesh_bounding_box_tag>;
    using MeshStorage = ecs::Archetype<Buffer>;
    using GPUMesh = ecs::Archetype<model::GPUOffsets, model::MeshStorageBufId>;
}

namespace engine {
    using ECS = ecs::build<archetypes::PerspCam, archetypes::OrthoCam, archetypes::Model, archetypes::Mesh, archetypes::MeshStorage, archetypes::GPUMesh>;
    extern ECS ecs;
}
