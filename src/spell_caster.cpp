#include "spell_caster.hpp"
#include "hitbox.hpp"

#include <string>
#include <tuple>
#include <vector>

namespace caster {
    std::vector<int> moving_spells = {};
    std::vector<std::tuple<uint, shapes::Circle>> circle_spells = {};

    void cast(uint spell_id, const Spell& spell, const Vector2& player_position, const Vector2& mouse_position) {
        auto reach = spell.get_reach();

        switch (spell.get_type()) {
            case Spell::AtMouse: {
                float radius = std::get<0>(reach);
                circle_spells.emplace_back(spell_id, shapes::Circle(mouse_position, radius));
                break;
            }
            case Spell::AtPlayer: {
                float radius = std::get<0>(reach);
                circle_spells.emplace_back(spell_id, shapes::Circle(player_position, radius));
                break;
            }
            case Spell::ToMouse: {
                break;
            }
        }
    }

    void tick(const SpellBook& spellbook) {
        for (auto [spell_id, circle] : circle_spells) {
            // TODO: check for collisions between enemies and the `circle` shape
        }
        circle_spells.clear();

        // TODO: tick `moving_spells`
    }

#ifdef DEBUG
    void draw_hitbox(float y) {
        for (auto [_, circle] : circle_spells) {
            circle.draw_3D(RED, y);
        }
    }
#endif
}
