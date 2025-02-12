#pragma once

#include "raylib.h"
#include <array>
#include <cassert>
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

// TODO: macro magic, same as for enemies
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
                    .speed = 5,
                    .duration = 0,
                },
            .element = Element::Ice,
            .cooldown = 20,
            .base_manacost = 20,
            .base_damage = 10,
        };
    };

#define EACH_SPELL(F, G)                                                                                               \
    G(FireWall)                                                                                                        \
    F(FrostNova)

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

    Spell(spells::Data&& spell, Rarity rarity, uint32_t level)
        : spell(spell), rarity(rarity), cooldown(get_info(spell).cooldown), current_cooldown(cooldown), level(level),
          experience(0), manacost(get_info(spell).base_manacost), damage(get_info(spell).base_damage) {
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

    spells::Tag get_spell_tag() const {
        return std::visit(
            [](auto&& spell) -> spells::Tag {
                using T = std::decay_t<decltype(spell)>;
                return spells::TagFromSpell<T>::tag;
            },
            spell);
    }

    rarity::Info get_rarity_info() const {
        return rarity::info[static_cast<int>(rarity)];
    }

    static Spell random(uint16_t max_level);

    ~Spell();
};

using SpellBook = std::vector<Spell>;
