#include "power_up.hpp"

#include "assets.hpp"
#include "raylib.h"
#include <random>

void PowerUp::draw(assets::Store& assets, const Rectangle& constraint) {
    assets.draw_texture(assets::PowerUpBackground, constraint);
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
