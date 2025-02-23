#pragma once

#include "input.hpp"
#include "raylib.h"

#include "assets.hpp"
#include "player.hpp"

namespace ui {
    struct Button {
        enum State {
            Normal,
            Hover,
        };

        std::function<void(State)> draw;
        shapes::Polygon hitbox;

        Button(shapes::Polygon&& poly, std::function<void(State)> draw);

        // draws the button and checks if button was pressed, returns true in that case
        bool update(Mouse& mouse);
    };

    struct Draggable {
        std::function<void(Vector2)> draw;
        // default origin
        Vector2 origin;

        // doesn't move, only initiates the dragging
        shapes::Polygon hitbox;

        Draggable(Vector2 origin, shapes::Polygon&& poly, std::function<void(Vector2)> draw);

        // returns a mouse position at which the component was dropped
        // doesn't call draw if it returns a Vector2
        std::optional<Vector2> update(Mouse& mouse);
    };
}

namespace hud {
    struct SpellBookUI {
        std::vector<std::pair<std::size_t, ui::Draggable>> spells;

        // doesn't assume where to draw, just a bunch of draw calls
        void draw(assets::Store& assets, const SpellBook& spellbook);
        void update(Mouse& mouse, std::optional<Vector2> screen_update);
    };

    void draw(assets::Store& assets, const Player& player, const SpellBook& spellbook, const Vector2& screen);
}

