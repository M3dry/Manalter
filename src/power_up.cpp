#include "power_up.hpp"

#include "print"
#include "raylib.h"
#include <random>

void PowerUp::draw(const Rectangle& constraint) {
    DrawRectangleRec(constraint, RED);
}

void PowerUp::draw_hover(const Rectangle& constraint) {
    DrawRectangleRec(constraint, BLUE);
}

void PowerUp::apply(PlayerStats& stats) {
    std::visit([&stats](auto&& arg) { arg.apply(stats); }, power_up);
}

PowerUp PowerUp::random() {
    static std::uniform_int_distribution<uint16_t> distr(0, static_cast<uint16_t>(powerups::Type::Size) - 1);

    return powerups::make_random(static_cast<powerups::Type>(distr(rng::get())));
}
