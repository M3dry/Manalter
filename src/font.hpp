#pragma once

#include <array>
#include <raylib.h>
#include <utility>

namespace font_manager {
    enum Fonts {
        Alagard,
        FontsSize,
    };

    constexpr unsigned char alagard_data[] = {
#embed "../assets/alagard.ttf"
    };

    static constexpr std::array<std::pair<const unsigned char*, int>, FontsSize> font_data = {
        std::make_pair(alagard_data, sizeof(alagard_data)),
    };

    enum Match {
        BestUnder,
        Exact,
    };

    void init();
    void clean();

    void draw_text(const char* text, Fonts font, unsigned int font_size, float spacing, Color tint, Vector2 pos,
                   Match match);
    std::pair<unsigned int, Vector2> max_font_size(Fonts font, const Vector2& max_dims, const char* text);
};
