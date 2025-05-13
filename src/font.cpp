#include "font.hpp"

#include <vector>

namespace font_manager {
    using SizedFonts = std::vector<std::pair<unsigned int, Font>>;
    std::array<SizedFonts, FontsSize> fonts;

    std::size_t best_match_under(Fonts font, unsigned int font_size) {
        auto& sized_font = fonts[font];

        std::size_t best_i = 0;
        for (std::size_t i = 1; i < sized_font.size(); i++) {
            if ((sized_font[i].first > sized_font[best_i].first || sized_font[best_i].first > font_size) &&
                sized_font[i].first <= font_size) {
                best_i = i;
            }
        }

        return best_i;
    }

    std::size_t exact_match(Fonts font, unsigned int font_size) {
        auto& sized_font = fonts[font];

        if (auto best = best_match_under(font, font_size); sized_font[best].first == font_size) {
            return best;
        }

        sized_font.emplace_back(font_size, LoadFontFromMemory(".ttf", font_data[font].first, font_data[font].second,
                                                              static_cast<int>(font_size), nullptr, 95));
        return sized_font.size() - 1;
    }

    void init() {
        for (std::size_t i = 0; i < FontsSize; i++) {
            fonts[i].emplace_back(32,
                                  LoadFontFromMemory(".ttf", font_data[i].first, font_data[i].second, 32, nullptr, 95));
        }
    }

    void clean() {
        for (auto& sized_font : fonts) {
            for (auto& [size, font] : sized_font) {
                UnloadFont(font);
            }
            sized_font.clear();
            sized_font.shrink_to_fit();
        }
    }

    void draw_text(const char* text, Fonts font, unsigned int font_size, float spacing, Color tint, Vector2 pos,
                   Match match) {
        auto& sized_font = fonts[font];
        std::size_t font_ix;
        switch (match) {
            case BestUnder:
                font_ix = best_match_under(font, font_size);
                break;
            case Exact:
                font_ix = exact_match(font, font_size);
                break;
        }

        DrawTextEx(fonts[font][font_ix].second, text, pos, static_cast<float>(font_size), spacing, tint);
    }

    std::pair<unsigned int, Vector2> max_font_size(Fonts font, const Vector2& max_dims, const char* text) {
        unsigned int lo = 1;
        unsigned int hi = 1000;
        unsigned int best_size = 0;
        Vector2 best_dims;

        while (lo <= hi) {
            unsigned int mid = (lo + hi) / 2;
            float spacing = static_cast<float>(mid) / 10.0f;
            Vector2 dims = MeasureTextEx(fonts[font][0].second, text, static_cast<float>(mid), spacing);

            if (dims.x <= max_dims.x && dims.y <= max_dims.y) {
                // Valid font size, try larger
                best_size = mid;
                best_dims = dims;
                lo = mid + 1;
            } else {
                // Too big, try smaller
                hi = mid - 1;
            }
        }

        return {best_size, best_dims};
    }
}
