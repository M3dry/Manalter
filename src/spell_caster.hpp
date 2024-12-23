#pragma once

#include "spell.hpp"
#include <raylib.h>
#include <sys/types.h>

namespace caster {
    void cast(uint spell_id, const Spell& spell, const Vector2& player_position, const Vector2& mouse_position);
    void tick(const SpellBook& spellbook);
#ifdef DEBUG
    void draw_hitbox(float y);
#endif
}
