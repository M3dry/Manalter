#include "power_up.hpp"

#include "assets.hpp"
#include "font.hpp"
#include "raylib.h"
#include <random>

template <powerups::__impl::DiscreteDistr T> void draw_text(const Rectangle& constraint, uint8_t value) {
    auto powerup_type = powerups::TypeToEnum<T>::powerup_type;

    auto [name_size, name_dims] = font_manager::max_font_size(
        font_manager::Alagard, Vector2{constraint.width * 0.9f, constraint.height * 0.15f}, T::name);
    font_manager::draw_text(T::name, font_manager::Alagard, name_size, static_cast<float>(name_size) / 10.0f, WHITE,
                            Vector2{
                                constraint.x + constraint.width / 2.0f - name_dims.x / 2.0f,
                                constraint.y + constraint.height * 0.01f,
                            },
                            font_manager::Exact);

    auto stat_text = std::format("+{}{}", value, powerup_type == powerups::Percentage ? "%" : " points");
    auto [stat_size, stat_dims] = font_manager::max_font_size(
        font_manager::Alagard, Vector2{constraint.width * 0.7f, constraint.height * 0.05f}, stat_text.data());
    font_manager::draw_text(stat_text.data(), font_manager::Alagard, stat_size, static_cast<float>(stat_size) / 10.0f,
                            WHITE,
                            Vector2{
                                constraint.x + constraint.width / 2.0f - stat_dims.x / 2.0f,
                                constraint.y + constraint.height / 2.0f - stat_dims.y / 2.0f,
                            },
                            font_manager::Exact);
}

void PowerUp::draw(assets::Store& assets, Rectangle constraint) {
    assets.draw_texture(assets::PowerUpBackground, constraint);

    constraint.x += 10;
    constraint.y += 10;
    constraint.width -= 20;
    constraint.height -= 20;
    std::visit(
        [&constraint](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;

            if (powerups::__impl::DiscreteDistr<T>) {
                draw_text<T>(constraint, arg.value);
            }
        },
        power_up);
}

void PowerUp::draw_hover(assets::Store& assets, Rectangle constraint) {
    DrawRectangleRec(constraint, BLUE);
}

void PowerUp::apply(PlayerStats& stats) {
    std::visit([&stats](auto&& arg) { arg.apply(stats); }, power_up);
}

PowerUp PowerUp::random() {
    static std::uniform_int_distribution<uint16_t> distr(0, static_cast<uint16_t>(powerups::Type::Size) - 1);

    return powerups::make_random(static_cast<powerups::Type>(distr(rng::get())));
}
