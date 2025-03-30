#include "spell.hpp"
#include <raylib.h>

spell::Info spells::get_info(const Data& data) {
    return std::visit([](auto&& arg) { return std::decay_t<decltype(arg)>::info; }, data);
}

SpellStats::SpellStats(const spell::Info& info, Rarity rarity, uint32_t level) {
    int rarity_multiplier = rarity::get_rarity_info(rarity).stat_multiplier * 100.0f;
    int level_multiplier = level * 5;
    manacost.add_points(info.base_manacost);
    manacost.add_percentage(std::max(rarity_multiplier - level_multiplier, 0));
    damage.add_points(info.base_damage);
    damage.add_percentage(rarity_multiplier + level_multiplier);
}

Spell Spell::random(uint16_t max_level) {
    return Spell(
        spells::create_spell(static_cast<spells::Tag>(GetRandomValue(0, static_cast<int>(spells::Tag::Size) - 1))),
        static_cast<Rarity>(GetRandomValue(0, static_cast<int>(Rarity::Size) - 1)), GetRandomValue(0, max_level));
}
