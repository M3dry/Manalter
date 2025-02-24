#pragma once

#include "spell.hpp"
#include "enemies_spawner.hpp"
#include <cstddef>
#include <raylib.h>

namespace caster {
    void clear();
    bool cast(std::size_t spell_id, const Spell& spell, const Vector2& player_position, const Vector2& mouse_position, const Enemies& enemies);
    void tick(const SpellBook& spellbook, Enemies& enemies, std::vector<ItemDrop>& item_drops);
#ifdef DEBUG
    void draw_hitbox(float y);
#endif
}
