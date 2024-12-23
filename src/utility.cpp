#include "utility.hpp"

#include <print>

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
