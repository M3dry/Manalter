#include "utility.hpp"

#include <chrono>
#include <cmath>
#include <print>
#include <random>

std::pair<int, Vector2> max_font_size(const Font& font, float spacing, const Vector2& max_dims, std::string_view text) {
    int font_size = 100;
    int min = 0;
    int max = -1;

    while (true) {
        Vector2 dims = MeasureTextEx(font, text.data(), font_size, spacing);

        if (dims.y < max_dims.y) {
            min = font_size;
        } else if (dims.y > max_dims.y) {
            max = font_size;
        } else
            return {font_size, dims};

        if (min == max) return {font_size, dims};

        if (max == -1) {
            font_size += 100;
        } else
            font_size = (min + max) / 2;
    }
}

// angle will be of range 0 - 360
// 0 degrees -> 12 o' clock
// 90 degrees -> 3 o' clock
// 180 degrees -> 6 o' clock
// 270 degrees -> 9 o' clock
float angle_from_point(const Vector2& point, const Vector2& origin) {
    return std::fmod(270 - std::atan2(origin.y - point.y, origin.x - point.x) * 180 / std::numbers::pi, 360);
}

Vector2 xz_component(const Vector3& vec) {
    return {vec.x, vec.z};
}

Vector2 mouse_xz_in_world(Ray mouse) {
    const float x = -mouse.position.y / mouse.direction.y;

    return (Vector2){mouse.position.x + x * mouse.direction.x, mouse.position.z + x * mouse.direction.z};
}

float wrap(float value, float modulus) {
    return value - modulus * std::floor(value / modulus);
}

Color lerp_color(Color start, Color end, float factor) {
    if (factor < 0.0f) {
        return start;
    } else if (factor > 1.0f) {
        return end;
    }

    return Color{
        .r = (unsigned char)((1.0f - factor) * start.r + factor * end.r),
        .g = (unsigned char)((1.0f - factor) * start.g + factor * end.g),
        .b = (unsigned char)((1.0f - factor) * start.b + factor * end.b),
        .a = (unsigned char)((1.0f - factor) * start.a + factor * end.a),
    };
}

void arena::loop_around(float& x, float& y) {
    if (x > ARENA_WIDTH / 2.0f) x -= ARENA_WIDTH;
    if (y > ARENA_HEIGHT / 2.0f) y -= ARENA_HEIGHT;
    if (x < -ARENA_HEIGHT / 2.0f) x += ARENA_WIDTH;
    if (y < -ARENA_HEIGHT / 2.0f) y += ARENA_HEIGHT;
}

std::mt19937 rng_gen(std::chrono::steady_clock::now().time_since_epoch().count());

std::mt19937& rng::get() {
    return rng_gen;
}
