#pragma once

#include "hitbox.hpp"
#include <random>
#include <string_view>
#include <utility>

#include <raylib.h>

#define TICKS 20
#define ARENA_WIDTH 5000
#define ARENA_HEIGHT 5000

std::pair<int, Vector2> max_font_size(const Font& font, float spacing, const Vector2& max_dims, std::string_view text);

float angle_from_point(const Vector2& point, const Vector2& origin);
Vector2 xz_component(const Vector3& vec);
Vector2 mouse_xz_in_world(Ray mouse);
float wrap(float value, float modulus);
Color lerp_color(Color start, Color end, float factor);

namespace arena {
    namespace _impl {
        constexpr Rectangle plane_to_rectangle(Vector2 origin, float width, float height) {
            return Rectangle{-width / 2.0f + origin.x, -height / 2.0f + origin.y, width, height};
        }
    }

    constexpr Rectangle arena_rec = Rectangle{ .x = -ARENA_WIDTH/2.0f, .y = -ARENA_HEIGHT/2.0f, .width = ARENA_WIDTH, .height = ARENA_HEIGHT };

    const shapes::Polygon center(_impl::plane_to_rectangle({ 0.0f, 0.0f }, ARENA_WIDTH, ARENA_HEIGHT));
    constexpr Vector2 center_origin = { 0.0f, 0.0f };

    const shapes::Polygon top(_impl::plane_to_rectangle({ARENA_WIDTH, ARENA_HEIGHT}, ARENA_WIDTH, ARENA_HEIGHT));
    const shapes::Polygon left(_impl::plane_to_rectangle({-ARENA_WIDTH, 0.0f}, ARENA_WIDTH, ARENA_HEIGHT));
    const shapes::Polygon right(_impl::plane_to_rectangle({ARENA_WIDTH, 0.0f}, ARENA_WIDTH, ARENA_HEIGHT));
    const shapes::Polygon bottom(_impl::plane_to_rectangle({ 0.0f, -ARENA_HEIGHT }, ARENA_WIDTH, ARENA_HEIGHT));
    constexpr Vector2 top_origin = {ARENA_WIDTH, ARENA_HEIGHT};
    constexpr Vector2 left_origin = {-ARENA_WIDTH, 0.0f};
    constexpr Vector2 right_origin = {ARENA_WIDTH, 0.0f};
    constexpr Vector2 bottom_origin = { 0.0f, -ARENA_HEIGHT };

    const shapes::Polygon top_right(_impl::plane_to_rectangle({ ARENA_WIDTH, ARENA_HEIGHT }, ARENA_WIDTH, ARENA_HEIGHT));
    const shapes::Polygon bottom_right(_impl::plane_to_rectangle({ ARENA_WIDTH, -ARENA_HEIGHT }, ARENA_WIDTH, ARENA_HEIGHT));
    constexpr Vector2 top_right_origin = { ARENA_WIDTH, ARENA_HEIGHT };
    constexpr Vector2 bottom_right_origin = { ARENA_WIDTH, -ARENA_HEIGHT };

    const shapes::Polygon top_left(_impl::plane_to_rectangle({ -ARENA_WIDTH, ARENA_HEIGHT }, ARENA_WIDTH, ARENA_HEIGHT));
    const shapes::Polygon bottom_left(_impl::plane_to_rectangle({ -ARENA_WIDTH, -ARENA_HEIGHT }, ARENA_WIDTH, ARENA_HEIGHT));
    constexpr Vector2 top_left_origin = { -ARENA_WIDTH, ARENA_HEIGHT };
    constexpr Vector2 bottom_left_origin = { -ARENA_WIDTH, -ARENA_HEIGHT };

    void loop_around(float& x, float& y);
}

namespace rng {
    std::mt19937& get();
};
