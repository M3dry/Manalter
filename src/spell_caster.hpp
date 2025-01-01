#pragma once

#include "spell.hpp"
#include <cstddef>
#include <raylib.h>

namespace caster {
    void cast(std::size_t spell_id, const Spell& spell, const Vector2& player_position, const Vector2& mouse_position);
    void tick(const SpellBook& spellbook);
#ifdef DEBUG
    void draw_hitbox(float y);
#endif
}
