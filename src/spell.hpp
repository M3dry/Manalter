#pragma once

#include <cstdint>
#include <utility>
#include <string>
#include <array>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <functional>

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

    static const std::array<std::string, NameSize> name_map;
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

    Spell(Name name, Rarity::Type rarity, uint16_t lvl);
    Spell(Spell&& s) noexcept;
    const Types& get_type() const;
    int get_cooldown() const;
    const std::string& get_icon() const;
    float get_rarity_multiplier() const;
    // rarity_range - inclusive from both sides,
    //                order doesn't matter, eg. Rare, Common -> Common, Uncommon, Rare
    //                                           Legendary, Legendary -> Legendary
    // lvl_range    - inclusive from both sides,
    //                order doesn't matter,
    static Spell create_random(std::pair<Rarity::Type, Rarity::Type> rarity_range, std::pair<uint16_t, uint16_t> lvl_range);

    // TODO: better random spells
    // static Spell create_random2(std::unordered_map<Rarity::Type, uint8_t> rarity_table, std::function<> lvl_) {
    //     
    // }
};

using SpellBook = std::vector<Spell>;
