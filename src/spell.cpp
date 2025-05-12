#include "spell.hpp"
#include "assets.hpp"
#include "font.hpp"
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
    manacost.add_percentage(static_cast<uint32_t>(std::max(rarity_multiplier - level_multiplier, 1.0f)));
    damage.add_points(info.base_damage);
    damage.add_percentage(static_cast<uint32_t>(rarity_multiplier + level_multiplier));
}

void SpellStats::lvl_increased() {
    manacost.set_percentage(std::max(static_cast<uint32_t>(1), manacost.get_percentage() - 5));
    damage.add_percentage(5.0f);
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
    exp += e;
    while (exp >= exp_to_next_lvl) {
        lvl++;
        exp -= exp_to_next_lvl;
        exp_to_next_lvl = exp_to_lvl(lvl + 1);
        stats.lvl_increased();
    }
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
    auto [name_size, _] = font_manager::max_font_size(
        font_manager::Alagard, Vector2{working_area.width, working_area.height * 0.4f}, info.name);
    font_manager::draw_text(info.name, font_manager::Alagard, name_size, static_cast<float>(name_size) / 10.0f, WHITE,
                            Vector2{working_area.x, working_area.y}, font_manager::Exact);

    working_area.y += working_area.height * 0.4f;
    working_area.height -= working_area.height * 0.4f;
    auto lvl_text =
        std::format("Level: {} ({:.1f}%)", lvl, static_cast<float>(exp) / static_cast<float>(exp_to_next_lvl) * 100.0f);
    auto [lvl_size, _] = font_manager::max_font_size(
        font_manager::Alagard, Vector2{working_area.width, working_area.height / 3.0f}, lvl_text.data());
    font_manager::draw_text(lvl_text.data(), font_manager::Alagard, lvl_size, static_cast<float>(lvl_size) / 10.0f,
                            WHITE, Vector2{working_area.x, working_area.y}, font_manager::Exact);

    auto stats_text = std::format("Damage: {}; Manacost: {}; Cooldown: {}s", stats.damage.get(), stats.manacost.get(),
                                  static_cast<float>(cooldown) / TICKS);
    auto [stats_size, _] = font_manager::max_font_size(
        font_manager::Alagard, Vector2{working_area.width, working_area.height * 2.0f / 3.0f}, stats_text.data());
    font_manager::draw_text(stats_text.data(), font_manager::Alagard, stats_size,
                            static_cast<float>(stats_size) / 10.0f, WHITE,
                            Vector2{working_area.x, working_area.y + working_area.height / 3.0f}, font_manager::Exact);
}

uint64_t Spell::exp_to_lvl(uint32_t lvl) {
    return 100 * static_cast<uint32_t>(std::pow(lvl, 2));
}

Spell Spell::random(uint32_t max_level) {
    std::uniform_int_distribution<uint32_t> levelDist(0, max_level);

    static std::discrete_distribution<uint8_t> dist({54, 28, 12, 5, 1});

    return Spell(
        spells::create_spell(static_cast<spells::Tag>(GetRandomValue(0, static_cast<int>(spells::Tag::Size) - 1))),
        static_cast<Rarity>(dist(rng::get())), levelDist(rng::get()));
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
    seria_deser::serialize(lvl, out);
    seria_deser::serialize(exp, out);
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
