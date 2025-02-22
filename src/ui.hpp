#pragma once

#include "input.hpp"
#include "raylib.h"

#include "assets.hpp"
#include "player.hpp"

namespace hud {
    void draw(assets::Store& assets, const Player& player, const SpellBook& spellbook, const Vector2& screen);
}

namespace ui {
    struct Button {
        Texture2D normal_tex;
        std::optional<Texture2D> hover_tex;
        Vector2 origin;

        shapes::Polygon hitbox;

        // button was pressed if true
        bool tick(Mouse& mouse);
    };
}
