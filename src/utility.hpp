#pragma once

#include <string_view>
#include <utility>

#include <raylib.h>

std::pair<int, Vector2> max_font_size(const Font& font, float spacing, const Vector2& max_dims, std::string_view text);
