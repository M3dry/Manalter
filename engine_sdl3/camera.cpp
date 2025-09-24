#include "engine/camera.hpp"
#include "ecs.hpp"
#include "engine/config.hpp"

#include "ecs_c.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>

#include <format>
#include <imgui.h>

namespace engine::camera {
    inline glm::vec3 _forward(glm::quat quat) {
        return quat * glm::vec3{0.0f, 0.0f, -1.0f};
    }

    inline glm::vec3 _right(glm::quat quat) {
        return quat * glm::vec3{1.0f, 0.0f, 0.0f};
    }

    inline glm::vec3 _up(glm::quat quat) {
        return quat * glm::vec3{0.0f, 1.0f, 0.0f};
    }

    template <typename... tags> inline std::tuple<get_type_t<tags>&...> get_components(CamId cam_id) {
        auto v = ecs.get<ecs::Static<archetypes::OrthoCam, archetypes::PerspCam>, tags...>(cam_id);
        if (!v) assert(false && "add expections please");

        return *v;
    }

    inline glm::quat& get_orientation(CamId cam_id) {
        return std::get<0>(get_components<cam_orientation_tag>(cam_id));
    }

    inline glm::mat4 perspective(Perspective conf) {
        return glm::perspective(glm::radians(conf.fov), conf.aspect_ratio, conf.near, conf.far);
    }

    inline glm::mat4 orthographic(Orthographic conf) {
        return glm::ortho(-conf.halved_frustum_width, conf.halved_frustum_width, -conf.halved_frustum_height,
                          conf.halved_frustum_height, conf.near, conf.far);
    }

    PerspCamId perspective_camera(Perspective conf) {
        glm::mat4 proj_mat = perspective(conf);

        auto id = PerspCamId(ecs.static_emplace_entity<archetypes::PerspCam>(
            conf, glm::vec3{0.0f}, glm::quat{0.0f, 0.f, 0.0f, 0.0f}, glm::identity<glm::mat4>(), proj_mat, proj_mat, "",
            0.0f, 0.0f, glm::vec3{0.0f}));
        auto [name] = get_components<camera_name_tag>(id);
        name = std::to_string(id);

        return id;
    }

    OrthoCamId orthographic_camera(Orthographic conf) {
        glm::mat4 proj_mat = orthographic(conf);

        auto id = OrthoCamId(ecs.static_emplace_entity<archetypes::OrthoCam>(
            conf, glm::vec3{0.0f}, glm::quat{0.0f, 0.f, 0.0f, 0.0f}, glm::identity<glm::mat4>(), proj_mat, proj_mat, "",
            0.0f, 0.0f, glm::vec3{0.0f}));
        auto [name] = get_components<camera_name_tag>(id);
        name = std::to_string(id);

        return id;
    }

    void set_perspective(PerspCamId cam_id, Perspective new_conf) {
        auto [conf, view_mat, proj_mat, view_proj_mat] =
            *ecs.get<ecs::Static<archetypes::PerspCam>, Perspective, cam_view_tag, cam_proj_tag,
                     cam_view_proj_tag>(cam_id);
        conf = new_conf;
        proj_mat = perspective(conf);
        view_proj_mat = proj_mat * view_mat;
    }

    void set_orthographic(OrthoCamId cam_id, Orthographic new_conf) {
        auto [conf, view_mat, proj_mat, view_proj_mat] =
            *ecs.get<ecs::Static<archetypes::OrthoCam>, Orthographic, cam_view_tag, cam_proj_tag,
                     cam_view_proj_tag>(cam_id);
        conf = new_conf;
        proj_mat = orthographic(conf);
        view_proj_mat = proj_mat * view_mat;
    }

    glm::vec3 forward(CamId cam_id) {
        return _forward(get_orientation(cam_id));
    }

    glm::vec3 right(CamId cam_id) {
        return _right(get_orientation(cam_id));
    }

    glm::vec3 up(CamId cam_id) {
        return _up(get_orientation(cam_id));
    }

    void update_view_matrix(CamId cam_id, glm::quat orientation, glm::vec3 pos, glm::mat4& view_mat,
                            const glm::mat4& proj_mat, glm::mat4& view_proj_mat) {
        auto focus = pos + _forward(orientation);
        if constexpr (config::debug_mode) {
            auto [yaw, pitch] = get_components<yaw_tag, pitch_tag>(cam_id);
            auto forward_vec = _forward(orientation);

            yaw = glm::degrees(std::atan2(forward_vec.x, -forward_vec.z));
            pitch = glm::degrees(std::asin(glm::clamp(forward_vec.y, -1.0f, 1.0f)));
        }

        view_mat = glm::lookAt(pos, pos + _forward(orientation), _up(orientation));
        view_proj_mat = proj_mat * view_mat;
    }

    glm::mat4 view_matrix(CamId cam_id) {
        return std::get<0>(get_components<cam_view_tag>(cam_id));
    }

    glm::mat4 projection_matrix(CamId cam_id) {
        return std::get<0>(get_components<cam_proj_tag>(cam_id));
    }

    glm::mat4 view_projection_matrix(CamId cam_id) {
        return std::get<0>(get_components<cam_view_proj_tag>(cam_id));
    }

    glm::vec3 position(CamId cam_id) {
        auto [pos] = get_components<cam_position_tag>(cam_id);
        return pos;
    }

    void set_position(CamId cam_id, glm::vec3 new_position) {
        auto [pos, orientation, view_mat, proj_mat, view_proj_mat] =
            get_components<cam_position_tag, cam_orientation_tag, cam_view_tag, cam_proj_tag,
                           cam_view_proj_tag>(cam_id);
        pos = new_position;

        update_view_matrix(cam_id, orientation, pos, view_mat, proj_mat, view_proj_mat);
    }

    void move(CamId cam_id, glm::vec3 movement) {
        auto [pos, orientation, view_mat, proj_mat, view_proj_mat] =
            get_components<cam_position_tag, cam_orientation_tag, cam_view_tag, cam_proj_tag,
                           cam_view_proj_tag>(cam_id);
        pos += movement;

        update_view_matrix(cam_id, orientation, pos, view_mat, proj_mat, view_proj_mat);
    }

    void look_at(CamId cam_id, glm::vec3 target, glm::vec3 up) {
        auto [orientation, pos, view_mat, proj_mat, view_proj_mat, focus_point] =
            get_components<cam_orientation_tag, cam_position_tag, cam_view_tag, cam_proj_tag,
                           cam_view_proj_tag, focus_point_tag>(cam_id);
        orientation = glm::quatLookAt(glm::normalize(target - pos), up);
        if constexpr (config::debug_mode) {
            focus_point = target;
        }

        update_view_matrix(cam_id, orientation, pos, view_mat, proj_mat, view_proj_mat);
    }

    glm::quat rotate(glm::quat orientation, float yaw_delta, float pitch_delta) {
        glm::quat yaw_rot = glm::angleAxis(glm::radians(yaw_delta), glm::vec3{0.0f, 1.0f, 0.0f});
        glm::quat pitch_rot = glm::angleAxis(glm::radians(pitch_delta), _right(orientation));
        return glm::normalize(yaw_rot * pitch_rot *orientation);
    }

    glm::quat absolute_rotate(float yaw, float pitch) {
        glm::quat yaw_rot = glm::angleAxis(glm::radians(-yaw), glm::vec3{0.0f, 1.0f, 0.0f});
        glm::vec3 right = yaw_rot * glm::vec3{1.0f, 0.0f, 0.0f};
        glm::quat pitch_rot = glm::angleAxis(glm::radians(pitch), right);

        return glm::normalize(pitch_rot * yaw_rot);
    }

    void rotate(CamId cam_id, float yaw_delta, float pitch_delta) {
        auto [orientation, pos, view_mat, proj_mat, view_proj_mat] =
            get_components<cam_orientation_tag, cam_position_tag, cam_view_tag, cam_proj_tag,
                           cam_view_proj_tag>(cam_id);

        orientation = rotate(orientation, yaw_delta, pitch_delta);

        update_view_matrix(cam_id, orientation, pos, view_mat, proj_mat, view_proj_mat);
    }

    void absolute_rotate(CamId cam_id, float yaw, float pitch) {
        auto [orientation, pos, view_mat, proj_mat, view_proj_mat] =
            get_components<cam_orientation_tag, cam_position_tag, cam_view_tag, cam_proj_tag,
                           cam_view_proj_tag>(cam_id);

        orientation = absolute_rotate(yaw, pitch);

        update_view_matrix(cam_id, orientation, pos, view_mat, proj_mat, view_proj_mat);
    }

    void projection_section(CamId cam_id) {
        // TODO: maybe do something awful like this :P
        // union {
        //     Perspective persp;
        //     Orthographic ortho;
        // } conf = {.persp = Perspective{} };

        if (ECS::to_index<archetypes::OrthoCam>::value == ecs.get_archetype(cam_id)) {
            ImGui::SeparatorText(std::format("projection: {}", "Orthographic").c_str());
            auto [ortho_conf] = *ecs.get<ecs::Static<archetypes::OrthoCam>, Orthographic>(cam_id);

            ImGui::InputFloat("halved frustum width", &ortho_conf.halved_frustum_width);
            ImGui::InputFloat("halved frustum height", &ortho_conf.halved_frustum_height);
            ImGui::DragFloat("near", &ortho_conf.near, 0.1f, 0.0f, ortho_conf.far);
            ImGui::DragFloat("far", &ortho_conf.far, 0.2f, ortho_conf.near, +FLT_MAX);

            set_orthographic(OrthoCamId(cam_id), ortho_conf);
        } else {
            ImGui::SeparatorText(std::format("projection: {}", "Perspective").c_str());
            auto [persp_conf] = *ecs.get<ecs::Static<archetypes::PerspCam>, Perspective>(cam_id);

            ImGui::DragFloat("fov", &persp_conf.fov, 0.2f, 0.0f, 360.0f, "%.0f");
            ImGui::InputFloat("aspect ratio", &persp_conf.aspect_ratio);
            ImGui::DragFloat("near", &persp_conf.near, 0.1f, 0.0f, persp_conf.far);
            ImGui::DragFloat("far", &persp_conf.far, 0.2f, persp_conf.near, +FLT_MAX);

            set_perspective(PerspCamId(cam_id), persp_conf);
        }
    }

    void imgui_section(CamId cam_id, const char* header_prefix) {
        if constexpr (config::debug_mode) {
            auto [pos, orientation, name, yaw, pitch, focus_point, view_mat, proj_mat, view_proj_mat] =
                get_components<cam_position_tag, cam_orientation_tag, camera_name_tag, yaw_tag, pitch_tag, focus_point_tag,
                               cam_view_tag, cam_proj_tag, cam_view_proj_tag>(cam_id);

            if (ImGui::CollapsingHeader(std::format("{}{}", header_prefix, name.c_str()).c_str())) {
                ImGui::InputFloat3("position", glm::value_ptr(pos));
                ImGui::SliderFloat("yaw", &yaw, -180.0f, 180.0f, "%.1f");
                ImGui::SliderFloat("pitch", &pitch, -89.0f, 89.0f, "%.1f");
                ImGui::InputFloat3("focus", glm::value_ptr(focus_point));
                ImGui::SameLine();
                if (ImGui::Button("Set")) {
                    look_at(cam_id, focus_point);
                }

                projection_section(cam_id);

                orientation = absolute_rotate(yaw, pitch);
                update_view_matrix(cam_id, orientation, pos, view_mat, proj_mat, view_proj_mat);
            }
        } else {
            if (ImGui::CollapsingHeader(std::format("{}{}", header_prefix, ecs::Entity(cam_id)).c_str())) {
                auto [pos, orientation] = get_components<cam_position_tag, cam_orientation_tag>(cam_id);
                ImGui::InputFloat3("position", glm::value_ptr(pos));
                ImGui::InputFloat4("orientation", glm::value_ptr(orientation));

                projection_section(cam_id);
            }
        }
    }

    void full_imgui_window() {
        ImGui::Begin("Cameras");
        ecs.make_system_query<ecs::Static<archetypes::OrthoCam, archetypes::PerspCam>>().run<ecs::WithIDs>(
            [](ecs::Entity id) { imgui_section(CamId(id), "Camera: "); });
        ImGui::End();
    }
}
