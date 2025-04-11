#include "power_up.hpp"

#include "assets.hpp"
#include "raylib.h"
#include <random>

template <powerups::__impl::DiscreteDistr T> void draw_text(const Rectangle& constraint, uint8_t value) {
    DrawText(T::name, static_cast<int>(constraint.x + 10), static_cast<int>(constraint.y + 20), 20, WHITE);
    DrawText(std::format("{}", value).c_str(), static_cast<int>(constraint.x + 10), static_cast<int>(constraint.y + 40), 20, WHITE);
}

void PowerUp::draw(assets::Store& assets, const Rectangle& constraint) {
    assets.draw_texture(assets::PowerUpBackground, constraint);

    std::visit([&constraint](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        if (powerups::__impl::DiscreteDistr<T>) {
            draw_text<T>(constraint, arg.value);
        }
    }, power_up);
}

void PowerUp::draw_hover(assets::Store& assets, const Rectangle& constraint) {
    DrawRectangleRec(constraint, BLUE);
}

void PowerUp::apply(PlayerStats& stats) {
    std::visit([&stats](auto&& arg) { arg.apply(stats); }, power_up);
}

PowerUp PowerUp::random() {
    static std::uniform_int_distribution<uint16_t> distr(0, static_cast<uint16_t>(powerups::Type::Size) - 1);

    return powerups::make_random(static_cast<powerups::Type>(distr(rng::get())));
}
