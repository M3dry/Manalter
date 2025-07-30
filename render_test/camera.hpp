#pragma once

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <optional>
#include <variant>

class Camera {
  public:
    struct Ortho {
        glm::vec2 frustum_dims_halved;
    };
    struct Perspec {
        float fov;
        float aspect_ratio;
    };

    Camera() {}
    Camera(glm::vec3 position, glm::vec3 focus, glm::vec3 up, float fov, float aspect_ratio) {
        set_view(position, focus, up);
        set_perspective(fov, aspect_ratio);
    }
    Camera(glm::vec3 position, glm::vec3 focus, glm::vec3 up, glm::vec2 frustum_dims_halved) {
        set_view(position, focus, up);
        set_orthographic(frustum_dims_halved);
    }

    void set_view(glm::vec3 new_pos, glm::vec3 new_focus, glm::vec3 new_up) {
        position = new_pos;
        focus = new_focus;
        up = new_up;

        update_view();
    }

    void set_orthographic(glm::vec2 frustum_dims_halved) {
        projection_data.emplace<Ortho>(frustum_dims_halved);
        projection = glm::ortho(-frustum_dims_halved.x, frustum_dims_halved.x, -frustum_dims_halved.y,
                                frustum_dims_halved.y, 0.1f, 100.0f);
    }

    void set_perspective(float fov, float aspect_ratio) {
        projection_data.emplace<Perspec>(fov, aspect_ratio);
        projection = glm::perspective(glm::radians(fov), aspect_ratio, 0.1f, 100.0f);
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

    bool is_orthographic() const {
        return std::holds_alternative<Ortho>(projection_data);
    }

    bool is_perspective() const {
        return std::holds_alternative<Perspec>(projection_data);
    }

    std::optional<Ortho> get_ortho_data() {
        if (is_orthographic()) return std::nullopt;

        return std::get<Ortho>(projection_data);
    }

    std::optional<Perspec> get_perspec_data() {
        if (is_perspective()) return std::nullopt;

        return std::get<Perspec>(projection_data);
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

  private:
    glm::vec3 position;
    glm::vec3 focus;
    glm::vec3 up;

    std::variant<Ortho, Perspec> projection_data;

    glm::mat4 view;
    glm::mat4 projection;

    void update_view() {
        view = glm::lookAt(position, focus, up);
    }
};
