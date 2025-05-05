#pragma once

#include "hitbox.hpp"
#include "input.hpp"
#include "raylib.h"
#include "assets.hpp"
#include "player.hpp"
#include "spell.hpp"

#include <vector>

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

    template <typename... Opts>
    struct Draggable {
        std::function<void(Vector2, Opts...)> draw;
        // default origin
        Vector2 origin;

        // doesn't move, only initiates the dragging
        shapes::Polygon hitbox;

        Draggable(Vector2 origin, shapes::Polygon&& poly, std::function<void(Vector2, Opts...)> draw) : draw(draw), origin(origin), hitbox(std::move(poly)) {
        }

        // returns a mouse position at which the component was dropped
        // doesn't call draw if it returns a Vector2
        std::optional<Vector2> update(Mouse& mouse, Opts... opts) {
            if (!mouse.button_press || mouse.button_press->button != Mouse::Button::Left || !check_collision(hitbox, mouse.button_press->pressed_at)) {
                draw(origin, opts...);
                return std::nullopt;
            }

            if (mouse.button_press->released_at) return *mouse.button_press->released_at;

            draw(origin + (mouse.mouse_pos - mouse.button_press->pressed_at), opts...);
            return std::nullopt;
        }
    };
}

namespace hud {
    struct SpellBookUI {
        std::size_t page_size = 12;

        std::vector<ui::Draggable<assets::Store&, const Spell&, const SpellBookUI&>> hitboxes;
        // [first, second)
        std::pair<uint64_t, uint64_t> spells;

        Rectangle area;
        Vector2 spell_dims;

        SpellBookUI(Vector2 tile_dims, const SpellBook& spellbook, const Vector2& screen);

        // returns the coords of where the spell was dropped
        std::optional<std::pair<uint64_t, Vector2>> update(assets::Store& assets, const SpellBook& spellbook, Mouse& mouse, std::optional<Vector2> screen);

        void draw_spell(assets::Store& assets, Vector2 origin, const Spell& spell) const;
    };

    void draw(assets::Store& assets, const Player& player, const SpellBook& spellbook, const Vector2& screen);

    struct SpellBar {
        // dims.xy - top left corner
        // dim.z - height
        int8_t dragged(Vector2 point, Vector3 dims, uint8_t unlocked_count);
        void draw(Vector3 dims, assets::Store& assets, const SpellBook& spellbook, std::span<std::size_t> equipped);
    };
}
