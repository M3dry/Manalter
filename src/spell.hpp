#pragma once

#include "raylib.h"
#include <array>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace spells {
    enum struct Tag;
}

enum struct Element {
    Fire = 0,
    Ice,
    Shadow,
    Light,
    Size,
};

namespace spell {
    namespace movement {
        enum Point {
            Player,
            Mouse,
            ClosestEnemy,
        };

        enum struct Type {
            Beam,
            Circle,
        };

        struct Beam {
            Point origin;
            Point dest;
            uint16_t length;
            uint8_t width;
            // after how many units should the spell stop
            // 0 - no stopping, duration is ignored
            uint16_t stop_after;
            // ticks to wait after stopping
            uint16_t duration;
            uint8_t speed;
        };

        struct Circle {
            Point center;
            uint8_t initial_radius;
            uint16_t maximal_radius;
            uint8_t speed;
            // ticks to wait after reaching `maximal_radius`
            uint16_t duration;
        };

        template <movement::Type T> using _Movement = std::conditional_t<T == Type::Beam, Beam, Circle>;
        using Movement = std::variant<_Movement<Type::Beam>, _Movement<Type::Circle>>;
    }

    static const char* icon_path = "./assets/spell-icons";
    struct Info {
        spells::Tag tag;
        const char* name;
        const char* icon;

        movement::Movement movement;
        const Element element;
        const uint16_t cooldown;
        const uint16_t base_manacost;
        const uint32_t base_damage;
    };
}

template <typename T>
concept IsSpell = requires(T s) {
    { T::info } -> std::same_as<const spell::Info&>;
};

namespace spells {
    enum struct Tag {
        FireWall,
        FrostNova,
        Size,
    };

    template <Tag T> struct TagToType;

    struct FireWall {
        static constexpr spell::Info info = {
            .tag = Tag::FireWall,
            .name = "Fire Wall",
            .icon = "fire-wall.png",
            .movement =
                (spell::movement::Beam){
                    .origin = spell::movement::Player,
                    .dest = spell::movement::Mouse,
                    .length = 30,
                    .width = 5,
                    .stop_after = 50,
                    .duration = 0,
                    .speed = 5,
                },
            .element = Element::Fire,
            .cooldown = 10,
            .base_manacost = 30,
            .base_damage = 2,
        };
    };

    template <> struct TagToType<Tag::FireWall> {
        using value = FireWall;
    };

    struct FrostNova {
        static constexpr spell::Info info = {
            .tag = Tag::FrostNova,
            .name = "Frost Nova",
            .icon = "frost-nova.png",
            .movement =
                (spell::movement::Circle){
                    .center = spell::movement::Mouse,
                    .initial_radius = 0,
                    .maximal_radius = 15,
                    .speed = 5,
                    .duration = 0,
                },
            .element = Element::Ice,
            .cooldown = 20,
            .base_manacost = 20,
            .base_damage = 10,
        };
    };

    template <> struct TagToType<Tag::FrostNova> {
        using value = FrostNova;
    };

    template <size_t... Is>
    consteval std::array<spell::Info, static_cast<int>(Tag::Size)> _gen_infos_impl(std::index_sequence<Is...>) {
        return { TagToType<static_cast<Tag>(Is)>::value::info... };
    }

    consteval std::array<spell::Info, static_cast<int>(Tag::Size)> _gen_infos() {
        return _gen_infos_impl(std::make_index_sequence<static_cast<int>(Tag::Size)>{});
    }

    static constexpr std::array<spell::Info, static_cast<int>(Tag::Size)> infos = _gen_infos();
}

enum struct Rarity {
    Common = 0,
    Uncommon,
    Rare,
    Epic,
    Legendary,
    Size
};

namespace rarity {
    static const char* frame_path = "./assets/spell-icons/frames";

    struct Info {
        const char* frame;
        const Color color;
        const float stat_multiplier;
    };

    static const std::array<Info, static_cast<int>(Rarity::Size)> info = {
        (Info){
            .frame = "common.png",
            .color = (Color){124, 129, 129, 255},
            .stat_multiplier = 1.0f,
        },
        (Info){
            .frame = "uncommon.png",
            .color = (Color){5, 249, 203, 255},
            .stat_multiplier = 2.0f,
        },
        (Info){
            .frame = "rare.png",
            .color = (Color){5, 92, 249, 255},
            .stat_multiplier = 3.0f,
        },
        (Info){
            .frame = "epic.png",
            .color = (Color){140, 12, 152, 255},
            .stat_multiplier = 5.0f,
        },
        (Info){
            .frame = "legendary.png",
            .color = (Color){252, 12, 12, 255},
            .stat_multiplier = 10.0f,
        },
    };
}

struct Spell {
    template <IsSpell... Ss> using SpellData = std::variant<Ss...>;

    SpellData<spells::FireWall, spells::FrostNova> spell;
    /*Texture2D texture;*/

    Rarity rarity;
    uint16_t cooldown;
    uint16_t current_cooldown;

    uint32_t level;
    uint64_t experience;

    uint32_t manacost;
    uint64_t damage;

    template <IsSpell S>
    Spell(S&& spell, Rarity rarity, uint32_t level)
        : spell(spell), rarity(rarity), cooldown(S::info.cooldown), current_cooldown(cooldown), level(level),
          experience(0), manacost(S::info.base_manacost), damage(S::info.base_damage){
                                                       // TODO: random manacost, damage
                                                   };
    Spell(Spell&&) noexcept = default;
    Spell& operator=(Spell&&) noexcept = default;

    spell::Info get_spell_info() const {
        return std::visit(
            [](auto&& spell) {
                using T = std::decay_t<decltype(spell)>;
                return T::info;
            },
            spell);
    }

    rarity::Info get_rarity_info() const {
        return rarity::info[static_cast<int>(rarity)];
    }

    ~Spell();
};

using SpellBook = std::vector<Spell>;
