#pragma once

#include <utility>
#include <string>
#include <array>
#include <unordered_set>
#include <vector>

// very generic names, TODO: maybe come up with better ones
namespace Rarity {
    enum Type {
        Common = 0,
        Uncommon,
        Rare,
        Epic,
        Legendary,
        Size
    };

    static const std::string frame_path = "./assets/spell-icons/frames";
    static const std::array<std::string, Size> frames = {
        "common.png", // Common
        "uncommon.png", // Uncommon
        "rare.png", // Rare
        "epic.png", // Epic
        "legendary.png", // Legendary
    };
}

using Spell = struct Spell {
    enum Name {
        Fire_Wall = 0,
        Falling_Icicle,
        Lightning_Strike,
        Frost_Nova,
        Void_Implosion,
        Mana_Detonation,
        NameSize, // keep last
    };

    enum Type {
        AOE,
        AtMouse,
        AtPlayer,
        Fire,
        Ice,
        Lightning,
        DarkMagic,
    };

    using Types = std::unordered_set<Type>;

    static const std::array<Types, NameSize> type_map;
    // in ticks, doesn't change with rarity
    static const std::array<int, NameSize> cooldown_map;
    static const std::string icon_path;
    // just file name, path is `Spell::icon_path`
    static const std::array<std::string, NameSize> icon_map;
    // changes damage
    static const std::array<float, Rarity::Size> rarity_multiplier;

    Name name;
    Rarity::Type rarity;
    uint16_t lvl;
    // exp collected since level up
    uint32_t exp;
    // in ticks
    int curr_cooldown;

    Spell(Name name, Rarity::Type rarity, uint16_t lvl)
        : name(name), rarity(rarity), lvl(lvl), exp(0), curr_cooldown(0) {};

    const Types& get_type() const {
        return Spell::type_map[name];
    };

    int get_cooldown() const {
        return Spell::cooldown_map[name];
    };

    const std::string& get_icon() const {
        return Spell::icon_map[name];
    };

    float get_rarity_multiplier() const {
        return Spell::rarity_multiplier[rarity];
    };

    // rarity_range - inclusive from both sides,
    //                order doesn't matter, eg. Rare, Common -> Common, Uncommon, Rare
    //                                           Legendary, Legendary -> Legendary
    // lvl_range    - inclusive from both sides,
    //                order doesn't matter,
    static Spell random(std::pair<Rarity::Type, Rarity::Type> rarity_range, std::pair<uint16_t, uint16_t> lvl_range) {
        auto [r1, r2] = rarity_range;
        Rarity::Type rarity = static_cast<Rarity::Type>(GetRandomValue(std::min(r1, r2), std::max(r1, r2)));

        auto [l1, l2] = lvl_range;
        uint16_t lvl = GetRandomValue(std::min(l1, l2), std::max(l1, l2));

        return Spell(static_cast<Name>(GetRandomValue(0, NameSize - 1)), rarity, lvl);
    };
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

using SpellBook = std::vector<Spell>;
