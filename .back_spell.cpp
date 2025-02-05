#include "spell.hpp"

#include "raylib.h"
#include <sys/types.h>

const std::array<std::string_view, Spell::NameSize> Spell::name_map = {
    "Fire Wall", "Falling Icicle", "Lightning Strike", "Frost Nova", "Void Implosion", "Mana Detonation",
};

const std::array<Element, Spell::NameSize> Spell::element_map = {
    Element::Fire,      // Fire Wall
    Element::Ice,       // Falling Icicle
    Element::Light, // Lightning Strike
    Element::Ice,       // Frost Nova
    Element::Shadow, // Void Implosion
    Element::Light, // Mana Detonation
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

const std::array<uint32_t, Spell::NameSize> Spell::damage_map = {
    5,        // Fire Wall
    5,   // Falling Icicle
    5, // Lightning Strike
    5,       // Frost Nova
    5,   // Void Implosion
    5,  // Mana Detonation
};

const std::array<std::pair<uint32_t, uint32_t>, Spell::NameSize> Spell::damage_randomization_map = {
    std::make_pair(1, 5),        // Fire Wall
    std::make_pair(1, 5),   // Falling Icicle
    std::make_pair(1, 5), // Lightning Strike
    std::make_pair(1, 5),       // Frost Nova
    std::make_pair(1, 5),   // Void Implosion
    std::make_pair(1, 5),  // Mana Detonation
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

const std::array<std::variant<Spell::SpellCircle, Spell::SpellToPoint>, Spell::NameSize> Spell::reach_map = {
    (Spell::SpellToPoint){
        .point = Spell::Point::Mouse,
        .length = 30,
        .width = 5,
        .stop_after = 50,
        .duration = 30,
        .speed = 5,
    }, // Fire Wall
    (Spell::SpellCircle){
        .point = Spell::Point::Mouse,
        .init_radius = 5,
        .max_radius = 5
    }, // Falling Icicle
    (Spell::SpellCircle){
        .point = Spell::Point::NearestEnemy,
        .max_radius = 1
    }, // Lightning Strike
    (Spell::SpellCircle){
        .point = Spell::Point::Mouse,
        .max_radius = 15,
        .speed = 5,
    }, // Frost Nova
    (Spell::SpellCircle){
        .point = Spell::Point::NearestEnemy,
        .max_radius = 5,
        .speed = 5,
    }, // Void Implosion
    (Spell::SpellCircle){
        .point = Spell::Point::Player,
        .max_radius = 20,
        .speed = 4,
    }, // Mana Detonation
};

Spell::Spell(Name name, Rarity::Type rarity, uint16_t lvl)
    : name(name), rarity(rarity), lvl(lvl), exp(0), manacost(manacost_map[name]), curr_cooldown(0), damage(damage_map[name]) {
    auto [minM, maxM] = get_manacost_randomization();
    manacost += GetRandomValue(minM, maxM);
    auto [minD, maxD] = get_damage_randomization();
    damage += GetRandomValue(minD, maxD);
}

const Color& Spell::get_rarity_color() const {
    return Rarity::color[static_cast<int>(rarity)];
}

std::string_view Spell::get_name() const {
    return Spell::name_map[name];
}

const Element& Spell::get_element() const {
    return Spell::element_map[name];
};

int Spell::get_cooldown() const {
    return Spell::cooldown_map[name];
};

int Spell::get_base_manacost() const {
    return Spell::manacost_map[name];
};

const std::string& Spell::get_icon_path() const {
    return Spell::icon_map[name];
};

uint32_t Spell::get_base_damage() const {
    return Spell::damage_map[name];
}

const std::pair<uint32_t, uint32_t>& Spell::get_damage_randomization() const {
    return Spell::damage_randomization_map[name];
}

float Spell::get_rarity_multiplier() const {
    return Spell::rarity_multiplier[static_cast<int>(rarity)];
};

const std::pair<int, int>& Spell::get_manacost_randomization() const {
    return Spell::manacost_randomization_map[static_cast<int>(rarity)];
};

const std::variant<Spell::SpellCircle, Spell::SpellToPoint>& Spell::get_reach() const {
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
