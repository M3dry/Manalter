#include "utility.hpp"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <random>
#include <raylib.h>
#include <rlgl.h>

std::pair<int, Vector2> max_font_size(const Font& font, float spacing, const Vector2& max_dims, std::string_view text) {
    int font_size = 100;
    int min = 0;
    int max = -1;

    while (true) {
        Vector2 dims = MeasureTextEx(font, text.data(), static_cast<float>(font_size), spacing);

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
    return std::fmod(270 - std::atan2(origin.y - point.y, origin.x - point.x) * 180.0f / std::numbers::pi_v<float>,
                     360.0f);
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

std::string format_to_time(double time) {
    uint64_t total_seconds = static_cast<uint64_t>(time);
    uint64_t hours = total_seconds / 3600;
    uint64_t minutes = (total_seconds % 3600) / 60;
    uint64_t seconds = total_seconds % 60;

    return std::format("{:02}:{:02}:{:02}", hours, minutes, seconds);
}

Vector4 spellbook_and_tile_dims(Vector2 screen, Vector2 spellbook_dims, Vector2 tile_dims) {
    auto spellbook_width = screen.x * 0.3f;
    auto spellbook_height = height_from_ratio(spellbook_dims, spellbook_width);
    if (spellbook_height > screen.y * 0.95f) {
        spellbook_height = screen.y * 0.95f;
        spellbook_width = width_from_ratio(spellbook_dims, spellbook_height);
    }

    return Vector4{
        spellbook_width,
        spellbook_height,
        spellbook_width * 0.75f,
        height_from_ratio(tile_dims, spellbook_width * 0.75f),
    };
}

Vector3 wrap_lerp(Vector3 a, Vector3 b, float t) {
    float half_width  = ARENA_WIDTH  / 2.0f;
    float half_height = ARENA_HEIGHT / 2.0f;

    float dx = b.x - a.x;
    if (dx >  half_width)  b.x -= ARENA_WIDTH;
    if (dx < -half_width)  b.x += ARENA_WIDTH;
    float x = a.x + (b.x - a.x) * t;
    if (x < -half_width) x += ARENA_WIDTH;
    if (x >  half_width) x -= ARENA_WIDTH;

    float y = a.y + (b.y - a.y) * t;

    float dz = b.z - a.z;
    if (dz >  half_height) b.z -= ARENA_HEIGHT;
    if (dz < -half_height) b.z += ARENA_HEIGHT;
    float z = a.z + (b.z - a.z) * t;
    if (z < -half_height) z += ARENA_HEIGHT;
    if (z >  half_height) z -= ARENA_HEIGHT;

    return Vector3{ x, y, z };
}

void arena::loop_around(float& x, float& y) {
    if (x > ARENA_WIDTH / 2.0f) x -= ARENA_WIDTH;
    if (y > ARENA_HEIGHT / 2.0f) y -= ARENA_HEIGHT;
    if (x < -ARENA_HEIGHT / 2.0f) x += ARENA_WIDTH;
    if (y < -ARENA_HEIGHT / 2.0f) y += ARENA_HEIGHT;
}

std::mt19937 rng_gen(static_cast<unsigned long>(std::chrono::steady_clock::now().time_since_epoch().count()));

std::mt19937& rng::get() {
    return rng_gen;
}
