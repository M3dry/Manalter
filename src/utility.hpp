#pragma once

#include <cstdint>
#include <string_view>
#include <utility>

#include <raylib.h>

#define TICKS 20
#define ARENA_WIDTH 1000
#define ARENA_HEIGHT 1000

std::pair<int, Vector2> max_font_size(const Font& font, float spacing, const Vector2& max_dims, std::string_view text);

float angle_from_point(const Vector2& point, const Vector2& origin);
Vector2 xz_component(const Vector3& vec);
Vector2 mouse_xz_in_world(Ray mouse);
