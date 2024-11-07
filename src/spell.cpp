#include "spell.hpp"

#include "raylib.h"

const std::array<std::string, Spell::NameSize> Spell::name_map = {
    "Fire Wall",
    "Falling Icicle",
    "Lightning Strike",
    "Frost Nova",
    "Void Implosion",
    "Mana Detonation",
};

const std::array<Spell::Types, Spell::NameSize> Spell::type_map = {
    std::unordered_set({ Spell::AOE, Spell::AtMouse, Spell::Fire }), // Fire Wall
    std::unordered_set({ Spell::AtMouse, Spell::Ice }), // Falling Icicle
    std::unordered_set({ Spell::AtMouse, Spell::Lightning }), // Lightning Strike
    std::unordered_set({ Spell::AOE,  Spell::AtMouse, Spell::Ice }), // Frost Nova
    std::unordered_set({ Spell::AOE, Spell::AtMouse, Spell::DarkMagic }), // Void Implosion
    std::unordered_set({ Spell::AOE, Spell::AtPlayer, Spell::DarkMagic }), // Mana Detonation
};

const std::array<int, Spell::NameSize> Spell::cooldown_map = {
    10, // Fire Wall
    10, // Falling Icicle
    15, // Lightning Strike
    20, // Frost Nova
    40, // Void Implosion
    40, // Mana Detonation
};

const std::string Spell::icon_path = "./assets/spell-icons";
const std::array<std::string, Spell::NameSize> Spell::icon_map = {
    "fire-wall.png", // Fire Wall
    "falling-icicle.png", // Falling Icicle
    "lightning-strike.png", // Lightning Strike
    "frost-nova.png", // Frost Nova
    "void-implosion.png", // Void Implosion
    "mana-detonation.png", // Mana Detonation
};

const std::array<float, Rarity::Size> Spell::rarity_multiplier = {
    1.0f, // Common
    1.1f, // Uncommon
    1.3f, // Rare
    1.6f, // Epic
    2.0f, // Legendary
};

Spell::Spell(Name name, Rarity::Type rarity, uint16_t lvl)
    : name(name), rarity(rarity), lvl(lvl), exp(0), curr_cooldown(0) {}
Spell::Spell(Spell&& s) noexcept
    : name(std::move(s.name)), rarity(s.rarity), lvl(s.lvl),
      exp(s.exp), curr_cooldown(s.curr_cooldown) {}

const Spell::Types& Spell::get_type() const {
    return Spell::type_map[name];
};

int Spell::get_cooldown() const {
    return Spell::cooldown_map[name];
};

const std::string& Spell::get_icon() const {
    return Spell::icon_map[name];
};

float Spell::get_rarity_multiplier() const {
    return Spell::rarity_multiplier[rarity];
};

Spell Spell::create_random(std::pair<Rarity::Type, Rarity::Type> rarity_range, std::pair<uint16_t, uint16_t> lvl_range) {
    auto [r1, r2] = rarity_range;
    Rarity::Type rarity = static_cast<Rarity::Type>(GetRandomValue(std::min(r1, r2), std::max(r1, r2)));

    auto [l1, l2] = lvl_range;
    uint16_t lvl = GetRandomValue(std::min(l1, l2), std::max(l1, l2));

    return Spell(static_cast<Name>(GetRandomValue(0, NameSize - 1)), rarity, lvl);
};
