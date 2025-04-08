#pragma once

#include "effects.hpp"
#include "raylib.h"
#include "stats.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

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
            uint8_t initial_radius = 0;
            uint16_t maximal_radius;
            uint8_t increase_duration = 0;
            // ticks to wait after reaching `maximal_radius`
            uint16_t duration = 0;
        };

        using Movement = std::variant<Beam, Circle>;
    }

    static const char* icon_path = "./assets/spell-icons";
    struct Info {
        const char* name;
        const char* icon;

        movement::Movement movement;
        Effect effect;
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
    struct FireWall {
        static constexpr spell::Info info = {
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

    struct FrostNova {
        static constexpr spell::Info info = {
            .name = "Frost Nova",
            .icon = "frost-nova.png",
            .movement =
                (spell::movement::Circle){
                    .center = spell::movement::Mouse,
                    .initial_radius = 0,
                    .maximal_radius = 15,
                    .increase_duration = 5,
                    .duration = 0,
                },
            .element = Element::Ice,
            .cooldown = 20,
            .base_manacost = 20,
            .base_damage = 5,
        };
    };

    struct FallingIcicile {
        static constexpr spell::Info info = {
            .name = "Falling Icicle",
            .icon = "falling-icicle.png",
            .movement =
                (spell::movement::Circle){
                    .center = spell::movement::Mouse,
                    .initial_radius = 3,
                    .maximal_radius = 3,
                },
            .element = Element::Ice,
            .cooldown = 10,
            .base_manacost = 50,
            .base_damage = 20,
        };
    };

    struct LightningStrike {
        static constexpr spell::Info info = {
            .name = "Lightning Strike",
            .icon = "lightning-strike.png",
            .movement =
                (spell::movement::Circle){
                    .center = spell::movement::ClosestEnemy,
                    .initial_radius = 1,
                    .maximal_radius = 1,
                },
            .element = Element::Light,
            .cooldown = 15,
            .base_manacost = 50,
            .base_damage = 30,
        };
    };

    struct VoidImplosion {
        static constexpr spell::Info info = {
            .name = "Void Implosion",
            .icon = "void-implosion.png",
            .movement =
                (spell::movement::Circle){
                    .center = spell::movement::ClosestEnemy,
                    .initial_radius = 15,
                    .maximal_radius = 5,
                    .increase_duration = 5,
                },
            .effect = effect::Plosion{
                .type = effect::Plosion::Im,
                .radius = 40.0f,
                .particle_count = 350,
                .particle_size_scale = 0.1f,
                .floor_y = 0.0f,
                .lifetime = { 0.1f, 0.7f},
                .velocity_scale = { 70.0f, 90.0f },
                .acceleration = {200.0f, 200.0f, 200.0f },
                .color = { {PURPLE, 80.0f}, { BLACK, 130.0f} },
            },
            .element = Element::Shadow,
            .cooldown = 40,
            .base_manacost = 75,
            .base_damage = 20,
        };
    };

    struct ManaDetonation {
        static constexpr spell::Info info = {
            .name = "Mana Detonation",
            .icon = "mana-detonation.png",
            .movement =
                (spell::movement::Circle){
                    .center = spell::movement::Player,
                    .maximal_radius = 20,
                    .increase_duration = 4,
                },
            .element = Element::Light,
            .cooldown = 40,
            .base_manacost = 60,
            .base_damage = 30,
        };
    };

#define EACH_SPELL(F, G)                                                                                               \
    G(FireWall)                                                                                                        \
    F(FrostNova)                                                                                                       \
    F(FallingIcicile)                                                                                                  \
    F(LightningStrike)                                                                                                 \
    F(VoidImplosion)

    enum class Tag {
#define SPELL_TAG_FIRST(name) name = 0,
#define SPELL_TAG(name) name,
        EACH_SPELL(SPELL_TAG, SPELL_TAG_FIRST) Size
#undef SPELL_TAG_FIRST
#undef SPELL_TAG_
    };

    using Data = std::variant<
#define SPELL_VARIANT_FIRST(name) name
#define SPELL_VARIANT(name) , name
        EACH_SPELL(SPELL_VARIANT, SPELL_VARIANT_FIRST)
#undef SPELL_VARIANT_FIRST
#undef SPELL_VARIANT
        >;

#define SPELL_CONCEPT(name) static_assert(IsSpell<name>);
    EACH_SPELL(SPELL_CONCEPT, SPELL_CONCEPT)
#undef SPELL_CONCEPT

    template <Tag T> struct SpellFromTag;

#define TAG_SPECIALIZE(name)                                                                                           \
    template <> struct SpellFromTag<Tag::name> {                                                                       \
        using Type = spells::name;                                                                                     \
    };

    EACH_SPELL(TAG_SPECIALIZE, TAG_SPECIALIZE)

#undef TAG_SPECIALIZE

    template <typename T> struct TagFromSpell;

#define SPELL_SPECIALIZE(name)                                                                                         \
    template <> struct TagFromSpell<name> {                                                                            \
        static constexpr Tag tag = Tag::name;                                                                          \
    };

    EACH_SPELL(SPELL_SPECIALIZE, SPELL_SPECIALIZE)

#undef SPELL_SPECIALIZE

    inline Data create_spell(Tag tag) {
        switch (tag) {
#define SPELL_CASE(name)                                                                                               \
    case Tag::name:                                                                                                    \
        return spells::name();
            EACH_SPELL(SPELL_CASE, SPELL_CASE)
#undef SPELL_CASE
            case Tag::Size:
                assert(false && "I hate you");
        }

        std::unreachable();
    }

    static constexpr std::array<spell::Info, static_cast<std::size_t>(Tag::Size)> infos = {
#define SPELL_INFO(name) name::info,
        EACH_SPELL(SPELL_INFO, SPELL_INFO)
#undef SPELL_INFO
    };
#undef EACH_ENEMY

    spell::Info get_info(const Data& data);
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

    inline rarity::Info get_rarity_info(const Rarity& rarity) {
        return rarity::info[static_cast<int>(rarity)];
    }
}

struct SpellStats {
    Stat<uint32_t, 0, 0> manacost;
    Stat<uint64_t, 0, 0> damage;

    SpellStats(const spell::Info& info, Rarity rarity, uint32_t level);
};

struct Spell {
    spells::Data spell;

    Rarity rarity;
    uint16_t cooldown;
    uint16_t current_cooldown;

    uint32_t level;
    uint64_t experience;

    SpellStats stats;

    Spell(spells::Data&& spell, Rarity rarity, uint32_t level)
        : spell(spell), rarity(rarity), cooldown(get_info(spell).cooldown), current_cooldown(0), level(level),
          experience(0), stats(get_info(spell), rarity, level) {};
    Spell(Spell&&) noexcept = default;
    Spell& operator=(Spell&&) noexcept = default;

    inline spell::Info get_spell_info() const {
        return get_info(spell);
    }

    inline spells::Tag get_spell_tag() const {
        return std::visit(
            [](auto&& spell) -> spells::Tag {
                using T = std::decay_t<decltype(spell)>;
                return spells::TagFromSpell<T>::tag;
            },
            spell);
    }

    static Spell random(uint16_t max_level);
};

using SpellBook = std::vector<Spell>;
