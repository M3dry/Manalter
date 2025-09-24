#pragma once

#include "ecs.hpp"
#include "engine/camera.hpp"
#include "engine/config.hpp"
#include "model_c.hpp"
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

TAG_BY_NAME(float, material_base_color_factor);

namespace engine::archetypes {
    template <typename... Ts>
    using Cam = typeset::set_union_t<
        type_set<Ts..., cam_position_tag, cam_orientation_tag, cam_view_tag, cam_proj_tag, cam_view_proj_tag>,
        std::conditional_t<config::debug_mode, type_set<camera_name_tag, yaw_tag, pitch_tag, focus_point_tag>,
                           type_set<>>,
        ecs::Archetype>;
    using PerspCam = Cam<camera::Perspective>;
    using OrthoCam = Cam<camera::Orthographic>;
}

namespace engine {
    using ECS =
        ecs::build_with_wrappers<type_set<ecs::wrapper<model::MeshId, archetypes::Mesh>>, archetypes::PerspCam,
                                 archetypes::OrthoCam, archetypes::Model, archetypes::Mesh, archetypes::GPUGroup>;
    extern ECS ecs;
}
