#pragma once

#include "util.hpp"

#include <format>
#include <optional>
#include <variant>

#include <imgui.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace engine {
    MAKE_WRAPPED_ID(PerspCamId)
    MAKE_WRAPPED_ID(OrthoCamId)

    class CamId {
      public:
          explicit CamId(ecs::Entity id) : id(id) {}
          CamId(const PerspCamId& persp_id) : id(persp_id) {}
          CamId(const OrthoCamId& persp_id) : id(persp_id) {}
          operator ecs::Entity() const {
              return id;
          }
      private:
        ecs::Entity id;
    };
}

namespace engine::camera {
    struct Perspective {
        float fov = 90.0f;
        float aspect_ratio = 16.0f / 9.0f;
        float near = 0.1f;
        float far = 100.0f;
    };

    struct Orthographic {
        float halved_frustum_width = 1.0f;
        float halved_frustum_height = 1.0f;
        float near = 0.1f;
        float far = 100.0f;
    };

    PerspCamId perspective_camera(Perspective conf);
    OrthoCamId orthographic_camera(Orthographic conf);

    glm::vec3 forward(CamId cam_id);
    glm::vec3 right(CamId cam_id);
    glm::vec3 up(CamId cam_id);

    glm::mat4 view_matrix(CamId cam_id);
    glm::mat4 projection_matrix(CamId cam_id);
    glm::mat4 view_projection_matrix(CamId cam_id);

    glm::vec3 position(CamId cam_id);

    void set_position(CamId cam_id, glm::vec3 new_position);
    void move(CamId cam_id, glm::vec3 movement);

    void look_at(CamId cam_id, glm::vec3 target, glm::vec3 up = {0.f, 1.0f, 0.0f});
    void rotate(CamId cam_id, float yaw_delta, float pitch_delta);

    void imgui_section(CamId cam_id, const char* header_prefix);
    void full_imgui_window();
}
