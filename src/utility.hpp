#pragma once

#include "hitbox.hpp"
#include <random>
#include <span>
#include <string_view>
#include <utility>

#include <raylib.h>

#define TICKS 20
#define ARENA_WIDTH 2000
#define ARENA_HEIGHT 2000

std::pair<int, Vector2> max_font_size(const Font& font, const Vector2& max_dims, std::string_view text);

float angle_from_point(const Vector2& point, const Vector2& origin);
Vector2 xz_component(const Vector3& vec);
Vector2 mouse_xz_in_world(Ray mouse);
float wrap(float value, float modulus);
std::string format_to_time(double time);
Vector4 spellbook_and_tile_dims(Vector2 screen, Vector2 spellbook_dims, Vector2 tile_dims);
Vector3 wrap_lerp(Vector3 a, Vector3 b, float t);
Mesh gen_mesh_quad(Vector3 p1, Vector3 p2, Vector3 p3, Vector3 p4);
void DrawBillboardCustom(Camera camera, Texture2D texture, Rectangle source, Vector3 position, Vector3 up, Vector2 size,
                         Vector2 origin, Vector3 rotations, Color tint);

inline Vector3 vec2_to_vec3(const Vector2& v) {
    return {v.x, 0.0f, v.y};
}

inline float width_from_ratio(Vector2 ratio, float height) {
    return (height * ratio.x) / ratio.y;
}

inline float height_from_ratio(Vector2 ratio, float width) {
    return (width * ratio.y) / ratio.x;
}

template <typename T, bool Zero = true> inline constexpr int sgn(T val) {
    T ret = (T(0) < val) - (val < T(0));
    if (!Zero && ret == 0) return static_cast<T>(1);
    return ret;
}

namespace arena {
    namespace _impl {
        constexpr Rectangle plane_to_rectangle(Vector2 origin, float width, float height) {
            return Rectangle{-width / 2.0f + origin.x, -height / 2.0f + origin.y, width, height};
        }
    }

    constexpr Rectangle arena_rec =
        Rectangle{.x = -ARENA_WIDTH / 2.0f, .y = -ARENA_HEIGHT / 2.0f, .width = ARENA_WIDTH, .height = ARENA_HEIGHT};

    const shapes::Polygon center(_impl::plane_to_rectangle({0.0f, 0.0f}, ARENA_WIDTH, ARENA_HEIGHT));
    constexpr Vector2 center_origin = {0.0f, 0.0f};

    const shapes::Polygon top(_impl::plane_to_rectangle({ARENA_WIDTH, ARENA_HEIGHT}, ARENA_WIDTH, ARENA_HEIGHT));
    const shapes::Polygon left(_impl::plane_to_rectangle({-ARENA_WIDTH, 0.0f}, ARENA_WIDTH, ARENA_HEIGHT));
    const shapes::Polygon right(_impl::plane_to_rectangle({ARENA_WIDTH, 0.0f}, ARENA_WIDTH, ARENA_HEIGHT));
    const shapes::Polygon bottom(_impl::plane_to_rectangle({0.0f, -ARENA_HEIGHT}, ARENA_WIDTH, ARENA_HEIGHT));
    constexpr Vector2 top_origin = {ARENA_WIDTH, ARENA_HEIGHT};
    constexpr Vector2 left_origin = {-ARENA_WIDTH, 0.0f};
    constexpr Vector2 right_origin = {ARENA_WIDTH, 0.0f};
    constexpr Vector2 bottom_origin = {0.0f, -ARENA_HEIGHT};

    const shapes::Polygon top_right(_impl::plane_to_rectangle({ARENA_WIDTH, ARENA_HEIGHT}, ARENA_WIDTH, ARENA_HEIGHT));
    const shapes::Polygon bottom_right(_impl::plane_to_rectangle({ARENA_WIDTH, -ARENA_HEIGHT}, ARENA_WIDTH,
                                                                 ARENA_HEIGHT));
    constexpr Vector2 top_right_origin = {ARENA_WIDTH, ARENA_HEIGHT};
    constexpr Vector2 bottom_right_origin = {ARENA_WIDTH, -ARENA_HEIGHT};

    const shapes::Polygon top_left(_impl::plane_to_rectangle({-ARENA_WIDTH, ARENA_HEIGHT}, ARENA_WIDTH, ARENA_HEIGHT));
    const shapes::Polygon bottom_left(_impl::plane_to_rectangle({-ARENA_WIDTH, -ARENA_HEIGHT}, ARENA_WIDTH,
                                                                ARENA_HEIGHT));
    constexpr Vector2 top_left_origin = {-ARENA_WIDTH, ARENA_HEIGHT};
    constexpr Vector2 bottom_left_origin = {-ARENA_WIDTH, -ARENA_HEIGHT};

    void loop_around(float& x, float& y);
}

namespace rng {
    std::mt19937& get();
};
