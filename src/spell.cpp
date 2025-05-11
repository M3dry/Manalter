#include "spell.hpp"
#include "assets.hpp"
#include "particle_system.hpp"
#include "seria_deser.hpp"
#include "utility.hpp"
#include <random>
#include <raylib.h>
#include <type_traits>

spell::Info spells::get_info(const Data& data) {
    return std::visit([](auto&& arg) { return std::decay_t<decltype(arg)>::info; }, data);
}

SpellStats::SpellStats() {
}

SpellStats::SpellStats(const spell::Info& info, Rarity rarity, uint32_t level) {
    float rarity_multiplier = rarity::get_rarity_info(rarity).stat_multiplier * 100.0f;
    float level_multiplier = static_cast<float>(level) * 5.0f;
    manacost.add_points(info.base_manacost);
    manacost.add_percentage(static_cast<uint32_t>(std::max(rarity_multiplier - level_multiplier, 0.0f)));
    damage.add_points(info.base_damage);
    damage.add_percentage(static_cast<uint32_t>(rarity_multiplier + level_multiplier));
}

void SpellStats::serialize(std::ostream& out) const {
    seria_deser::serialize(manacost, out);
    seria_deser::serialize(damage, out);
}

SpellStats SpellStats::deserialize(std::istream& in, version version) {
    SpellStats stats;
    stats.manacost = seria_deser::deserialize<decltype(stats.manacost)>(in, version);
    stats.damage = seria_deser::deserialize<decltype(stats.damage)>(in, version);

    return stats;
}

void Spell::add_exp(uint32_t e) {
    e += e;
}

void Spell::draw(Rectangle working_area, assets::Store& assets) {
    auto info = get_spell_info();

    assets.draw_texture(assets::SpellTileBackground, working_area);
    assets.draw_texture(get_spell_tag(), Rectangle{
                                             .x = working_area.x,
                                             .y = working_area.y,
                                             .width = working_area.height * 0.7f,
                                             .height = working_area.height * 0.7f,
                                         });

    DrawTexturePro(assets[assets::SpellTileRarityFrame],
                   Rectangle{
                       .x = 0.0f,
                       .y = 0.0f,
                       .width = static_cast<float>(assets[assets::SpellTileRarityFrame].width),
                       .height = static_cast<float>(assets[assets::SpellTileRarityFrame].height),
                   },
                   working_area, Vector2Zero(), 0.0f, rarity::get_rarity_info(rarity).color);

    working_area.x += working_area.height * 0.75f;
    working_area.width -= working_area.height * 0.75f;
    working_area.height = working_area.height * 0.7f;
    auto [size, _] = max_font_size(GetFontDefault(),
                                   Vector2{working_area.width, working_area.height * 0.4f}, info.name);
    DrawText(info.name, static_cast<int>(working_area.x), static_cast<int>(working_area.y), size, WHITE);

    working_area.y += working_area.height * 0.4f;
    working_area.height -= working_area.height * 0.4f;
    DrawRectangleRec(working_area, BLACK);
}

Spell Spell::random(uint32_t max_level) {
    std::uniform_int_distribution<uint32_t> levelDist(0, max_level);

    return Spell(
        spells::create_spell(static_cast<spells::Tag>(GetRandomValue(0, static_cast<int>(spells::Tag::Size) - 1))),
        static_cast<Rarity>(GetRandomValue(0, static_cast<int>(Rarity::Size) - 1)), levelDist(rng::get()));
}

void Spell::serialize(std::ostream& out) const {
    seria_deser::serialize(get_spell_tag(), out);

    std::visit(
        [&](const auto& arg) {
            if constexpr (SeriaDeser<std::decay_t<decltype(arg)>>) {
                seria_deser::serialize(arg, out);
            }
        },
        spell);

    seria_deser::serialize(rarity, out);
    seria_deser::serialize(level, out);
    seria_deser::serialize(experience, out);
    seria_deser::serialize(stats, out);
}

Spell Spell::deserialize(std::istream& in, version version) {
    auto data = spells::deserialize(in, version, seria_deser::deserialize<spells::Tag>(in, version));
    auto rarity = seria_deser::deserialize<Rarity>(in, version);
    auto level = seria_deser::deserialize<uint32_t>(in, version);
    auto experience = seria_deser::deserialize<uint64_t>(in, version);
    auto stats = seria_deser::deserialize<SpellStats>(in, version);

    return Spell(std::move(data), rarity, level, experience, stats);
}
