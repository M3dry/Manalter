#include "spell.hpp"

#include "raylib.h"

const std::array<std::string_view, Spell::NameSize> Spell::name_map = {
    "Fire Wall", "Falling Icicle", "Lightning Strike", "Frost Nova", "Void Implosion", "Mana Detonation",
};

const std::array<Spell::Type, Spell::NameSize> Spell::type_map = {
    Spell::ToMouse,  // Fire Wall
    Spell::AtMouse,  // Falling Icicle
    Spell::AtMouse,  // Lightning Strike
    Spell::AtMouse,  // Frost Nova
    Spell::AtMouse,  // Void Implosion
    Spell::AtPlayer, // Mana Detonation
};

const std::array<Spell::Element, Spell::NameSize> Spell::element_map = {
    Spell::Fire,      // Fire Wall
    Spell::Ice,       // Falling Icicle
    Spell::Lightning, // Lightning Strike
    Spell::Ice,       // Frost Nova
    Spell::DarkMagic, // Void Implosion
    Spell::DarkMagic, // Mana Detonation
};

const std::array<int, Spell::NameSize> Spell::cooldown_map = {
    10, // Fire Wall
    10, // Falling Icicle
    15, // Lightning Strike
    20, // Frost Nova
    40, // Void Implosion
    40, // Mana Detonation
};

const std::array<int, Spell::NameSize> Spell::manacost_map = {
    30, // Fire Wall
    50, // Falling Icicle
    40, // Lightning Strike
    20, // Frost Nova
    60, // Void Implosion
    50, // Mana Detonation
};

const std::string Spell::icon_path = "./assets/spell-icons";
const std::array<std::string, Spell::NameSize> Spell::icon_map = {
    "fire-wall.png",        // Fire Wall
    "falling-icicle.png",   // Falling Icicle
    "lightning-strike.png", // Lightning Strike
    "frost-nova.png",       // Frost Nova
    "void-implosion.png",   // Void Implosion
    "mana-detonation.png",  // Mana Detonation
};

const std::array<float, Rarity::Size> Spell::rarity_multiplier = {
    1.0f, // Common
    1.1f, // Uncommon
    1.3f, // Rare
    1.6f, // Epic
    2.0f, // Legendary
};

const std::array<std::pair<int, int>, Rarity::Size> Spell::manacost_randomization_map = {
    std::make_pair(0, 10),    // Common
    std::make_pair(-5, 5),    // Uncommon
    std::make_pair(-10, 0),   // Rare
    std::make_pair(-20, -5),  // Epic
    std::make_pair(-40, -10), // Legendary
};

const std::array<std::variant<Spell::Radius, Spell::DistanceWidth>, Spell::NameSize> Spell::reach_map = {
    std::make_tuple(100, 20, 20, 5), // Fire Wall
    (uint8_t)5,                      // Falling Icicle
    (uint8_t)0,                      // Lightning Strike
    (uint8_t)25,                     // Frost Nova
    (uint8_t)35,                     // Void Implosion
    (uint8_t)50,                     // Mana Detonation
};

Spell::Spell(Name name, Rarity::Type rarity, uint16_t lvl)
    : name(name), rarity(rarity), lvl(lvl), exp(0), manacost(manacost_map[name]), curr_cooldown(0) {
    auto [min, max] = get_manacost_randomization();
    manacost += GetRandomValue(min, max);
}

const Color& Spell::get_rarity_color() const {
    return Rarity::color[static_cast<int>(rarity)];
}

std::string_view Spell::get_name() const {
    return Spell::name_map[name];
}

Spell::Type Spell::get_type() const {
    return Spell::type_map[name];
};

const Spell::Element& Spell::get_element() const {
    return Spell::element_map[name];
};

int Spell::get_cooldown() const {
    return Spell::cooldown_map[name];
};

int Spell::get_manacost() const {
    return Spell::manacost_map[name];
};

const std::string& Spell::get_icon_path() const {
    return Spell::icon_map[name];
};

float Spell::get_rarity_multiplier() const {
    return Spell::rarity_multiplier[static_cast<int>(rarity)];
};

const std::pair<int, int>& Spell::get_manacost_randomization() const {
    return Spell::manacost_randomization_map[static_cast<int>(rarity)];
};

const std::variant<Spell::Radius, Spell::DistanceWidth>& Spell::get_reach() const {
    return Spell::reach_map[name];
}

Spell Spell::create_random(std::pair<Rarity::Type, Rarity::Type> rarity_range,
                           std::pair<uint16_t, uint16_t> lvl_range) {
    auto [r1, r2] = rarity_range;
    Rarity::Type rarity = static_cast<Rarity::Type>(GetRandomValue(std::min(r1, r2), std::max(r1, r2)));

    auto [l1, l2] = lvl_range;
    uint16_t lvl = GetRandomValue(std::min(l1, l2), std::max(l1, l2));

    return Spell(static_cast<Name>(GetRandomValue(0, NameSize - 1)), rarity, lvl);
};
