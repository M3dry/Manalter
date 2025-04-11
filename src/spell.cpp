#include "spell.hpp"
#include "utility.hpp"
#include <random>
#include <raylib.h>

spell::Info spells::get_info(const Data& data) {
    return std::visit([](auto&& arg) { return std::decay_t<decltype(arg)>::info; }, data);
}

SpellStats::SpellStats(const spell::Info& info, Rarity rarity, uint32_t level) {
    float rarity_multiplier = rarity::get_rarity_info(rarity).stat_multiplier * 100.0f;
    float level_multiplier = static_cast<float>(level) * 5.0f;
    manacost.add_points(info.base_manacost);
    manacost.add_percentage(static_cast<uint32_t>(std::max(rarity_multiplier - level_multiplier, 0.0f)));
    damage.add_points(info.base_damage);
    damage.add_percentage(static_cast<uint32_t>(rarity_multiplier + level_multiplier));
}

Spell Spell::random(uint32_t max_level) {
    std::uniform_int_distribution<uint32_t> levelDist(0, max_level);

    return Spell(
        spells::create_spell(static_cast<spells::Tag>(GetRandomValue(0, static_cast<int>(spells::Tag::Size) - 1))),
        static_cast<Rarity>(GetRandomValue(0, static_cast<int>(Rarity::Size) - 1)), levelDist(rng::get()));
}
