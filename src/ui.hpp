#pragma once

#include "raylib.h"

#include "assets.hpp"
#include "player.hpp"

namespace hud {
    void draw(assets::Store& assets, const Player& player, const SpellBook& spellbook, const Vector2& screen);
}

void draw_target();
