#pragma once

#include <array>
#include <cstdint>
#include <raylib.h>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

// `+` - strong against these elements
// `-` - weak against these elements
enum struct Element {
    Fire = 0,
    Ice,
    Shadow,
    Light,
    Size,
};


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
        "common.png",    // Common
        "uncommon.png",  // Uncommon
        "rare.png",      // Rare
        "epic.png",      // Epic
        "legendary.png", // Legendary
    };
    static const std::array<Color, Size> color = {
        (Color){124, 129, 129, 255}, // Common
        (Color){5, 249, 203, 255},   // Uncommon
        (Color){5, 92, 249, 255},    // Rare
        (Color){140, 12, 152, 255},  // Epic
        (Color){252, 12, 12, 255},   // Legendary
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
        NameSize,
    };

    static const std::array<std::string_view, NameSize> name_map;
    static const std::array<Element, NameSize> element_map;
    // in ticks, doesn't change with rarity
    static const std::array<int, NameSize> cooldown_map;
    // base values, get increases by `rarity_multiplier`
    // on spell creation the manacost is increased or decreased by a random amount based on the rarity
    static const std::array<int, NameSize> manacost_map;
    static const std::string icon_path;
    // just file name, path is `Spell::icon_path`
    static const std::array<std::string, NameSize> icon_map;
    // changes damage
    static const std::array<float, Rarity::Size> rarity_multiplier;
    static const std::array<std::pair<int, int>, Rarity::Size> manacost_randomization_map;

    // AtPlayer, AtMouse
    // 0 value makes it hit what is right under the cursor
    using Radius = uint8_t;
    enum struct Point {
        Player,
        Mouse,
        NearestEnemy,
    };

    struct SpellCircle {
        Point point;
        uint8_t init_radius = 0;
        uint8_t max_radius;
        // units per tick
        uint8_t speed = 0;
        // how many ticks to wait after reaching `max_radius` before removing spell
        uint16_t duration = 0;
    };

    struct SpellToPoint {
        Point point;
        uint16_t length;
        uint8_t width;
        // after how many units should the spell stop
        // 0 - no stopping
        uint16_t stop_after;
        uint16_t duration;
        // units per tick
        uint8_t speed;
    };
    static const std::array<std::variant<SpellCircle, SpellToPoint>, NameSize> reach_map;

    Name name;
    Rarity::Type rarity;
    uint16_t lvl;
    // exp collected since level up
    uint32_t exp;
    int manacost;
    // in ticks
    int curr_cooldown;

    Spell(Name name, Rarity::Type rarity, uint16_t lvl);
    Spell(Spell&& s) noexcept = default;
    Spell& operator=(Spell&&) noexcept = default;

    const Color& get_rarity_color() const;
    std::string_view get_name() const;
    const Element& get_element() const;
    int get_cooldown() const;
    int get_manacost() const;
    const std::string& get_icon_path() const;
    float get_rarity_multiplier() const;
    const std::pair<int, int>& get_manacost_randomization() const;
    const std::variant<Spell::SpellCircle, Spell::SpellToPoint>& get_reach() const;

    // rarity_range - inclusive from both sides,
    //                order doesn't matter, eg. Rare, Common -> Common, Uncommon, Rare
    //                                           Legendary, Legendary -> Legendary
    // lvl_range    - inclusive from both sides,
    //                order doesn't matter,
    static Spell create_random(std::pair<Rarity::Type, Rarity::Type> rarity_range,
                               std::pair<uint16_t, uint16_t> lvl_range);

    // TODO: better random spells
    // static Spell create_random2(std::unordered_map<Rarity::Type, uint8_t> rarity_table, std::function<> lvl_) {
    //
    // }
};

using SpellBook = std::vector<Spell>;
