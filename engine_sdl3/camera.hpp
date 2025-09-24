#pragma once

#include <format>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <optional>
#include <variant>

class Camera {
  public:
    struct Perspec {
        float fov{90.0f};
        float aspect_ratio{16.0f / 9.0f};
    };
    struct Ortho {
        glm::vec2 frustum_dims_halved{1.0f, 1.0f};
    };
    struct Freecam {
        float yaw = -90.0f;
        float pitch = 0.0f;
        glm::vec3 forward;
        glm::vec3 right;
        glm::vec3 up;

        Freecam(glm::vec3 world_up) {
            update(world_up);
        }

        void update(glm::vec3 world_up) {
            float yaw_rad = glm::radians(yaw);
            float pitch_rad = glm::radians(pitch);

            forward = glm::normalize(glm::vec3{std::cosf(yaw_rad) * std::cosf(pitch_rad), std::sinf(pitch_rad),
                                               std::sinf(yaw_rad) * std::cosf(pitch_rad)});
            right = glm::normalize(glm::cross(forward, world_up));
            up = glm::normalize(glm::cross(right, forward));
        }
    };

    Camera() : freecam_data(glm::vec3{0.0f, 1.0f, 0.0f}) {}
    Camera(glm::vec3 position, glm::vec3 focus, glm::vec3 up, float fov, float aspect_ratio) : freecam_data(up) {
        set_view(position, focus, up);
        set_perspective(fov, aspect_ratio);
    }
    Camera(glm::vec3 position, glm::vec3 focus, glm::vec3 up, glm::vec2 frustum_dims_halved) : freecam_data(up) {
        set_view(position, focus, up);
        set_orthographic(frustum_dims_halved);
    }

    void set_view(glm::vec3 new_pos, glm::vec3 new_focus, glm::vec3 new_up) {
        position = new_pos;
        focus = new_focus;
        up = new_up;

        update_view();
    }

    void set_perspective(float fov, float aspect_ratio) {
        projection_type = Perspective;
        perspec = (Perspec){fov, aspect_ratio};
        update_projection_perspective();
    }

    void set_orthographic(glm::vec2 frustum_dims_halved) {
        projection_type = Orthographic;
        ortho = {frustum_dims_halved};
        update_projection_orthographic();
    }

    void set_position(glm::vec3 new_pos) {
        position = new_pos;
        update_view();
    }

    void set_focus(glm::vec3 new_focus) {
        focus = new_focus;
        update_view();
    }

    void set_up(glm::vec3 new_up) {
        up = new_up;
        update_view();
    }

    void set_freecam(float yaw,  float pitch) {
        _is_freecam = true;

        freecam_data.yaw = yaw;
        freecam_data.pitch = pitch;
        freecam_data.update(up);

        update_view();
    }

    void enable_freecam() {
        _is_freecam = true;

        freecam_data.update(up);
        update_view();
    }

    bool is_orthographic() const {
        return projection_type == Orthographic;
    }

    bool is_perspective() const {
        return projection_type == Perspective;
    }

    bool is_freecam() const {
        return _is_freecam;
    }

    glm::vec3 get_position() {
        return position;
    }

    glm::vec3 get_focus() {
        return focus;
    }

    glm::vec3 get_up() {
        return up;
    }

    std::optional<Ortho> get_ortho_data() {
        if (is_orthographic()) return std::nullopt;

        return ortho;
    }

    std::optional<Perspec> get_perspec_data() {
        if (is_perspective()) return std::nullopt;

        return perspec;
    }

    Freecam get_freecam_data() {
        return freecam_data;
    }

    glm::mat4 apply_model(const glm::mat4& model) const {
        return get_view_projection() * model;
    }

    glm::mat4 get_view_projection() const {
        return projection * view;
    }

    glm::mat4 get_view() const {
        return view;
    }

    glm::mat4 get_projection() const {
        return projection;
    }

    void imgui_window(const char* name) {
        ImGui::Begin(std::format("Camera: {}", name).c_str());

        ImGui::InputFloat3("position", glm::value_ptr(position));
        ImGui::InputFloat3("focus", glm::value_ptr(focus));
        ImGui::InputFloat3("up", glm::value_ptr(up));

        ImGui::SeparatorText("Projection");

        ImGui::Combo("type", (int*)(&projection_type), "Perspective\0Orthographic\0\0");

        if (is_perspective()) {
            ImGui::SliderFloat("fov", &perspec.fov, 0.0f, 360.0f, "%.0f");
            ImGui::InputFloat("aspect ratio", &perspec.aspect_ratio, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);
        } else {
            ImGui::InputFloat2("halved frustum dimensions", glm::value_ptr(ortho.frustum_dims_halved), "%.0f");
        }

        ImGui::Checkbox("freecam", &_is_freecam);
        if (is_freecam()) {
            ImGui::SliderFloat("yaw", &freecam_data.yaw, -180.0f, 180.0f, "%.1f");
            ImGui::SliderFloat("pitch", &freecam_data.pitch, -89.0f, 89.0f, "%.1f");
        }

        ImGui::End();

        update_view();
        if (is_perspective()) {
            update_projection_perspective();
        } else {
            update_projection_orthographic();
        }
    }

  private:
    enum ProjectionType {
        Perspective = 0,
        Orthographic = 1,
    };

    glm::vec3 position;
    glm::vec3 focus;
    glm::vec3 up;

    ProjectionType projection_type;
    Perspec perspec{};
    Ortho ortho{};

    bool _is_freecam = false;
    Freecam freecam_data;

    glm::mat4 view;
    glm::mat4 projection;

    inline void update_view() {
        if (is_freecam()) {
            freecam_data.update(up);
            view = glm::lookAt(position, position + freecam_data.forward, freecam_data.up);
        } else {
            view = glm::lookAt(position, focus, up);
        }
    }
    inline void update_projection_perspective() {
        projection = glm::perspective(glm::radians(perspec.fov), perspec.aspect_ratio, 0.1f, 100.0f);
    }
    inline void update_projection_orthographic() {
        projection = glm::ortho(-ortho.frustum_dims_halved.x, ortho.frustum_dims_halved.x, -ortho.frustum_dims_halved.y,
                                ortho.frustum_dims_halved.y, 0.1f, 100.0f);
    }
};
