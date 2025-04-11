#pragma once

#include <cassert>
#include <concepts>
#include <cstdint>
#include <random>
#include <utility>
#include <variant>

#include "assets.hpp"
#include "stats.hpp"
#include "utility.hpp"

namespace powerups {
    enum PowerUpType {
        Percentage,
        Absolute,
    };

    namespace __impl {
        template <typename T>
        concept DiscreteDistr = requires(T t) {
            { T::name } -> std::convertible_to<const char*>;
            { T::min } -> std::convertible_to<uint8_t>;
            { T::max } -> std::convertible_to<uint8_t>;
            { T::step } -> std::convertible_to<uint8_t>;
            { T::weights } -> std::same_as<const double(&)[std::extent_v<decltype(T::weights)>]>;
            { t.value } -> std::convertible_to<uint8_t>;
        };

        template <typename T>
        concept HasRandom = requires() {
            { T::random() } -> std::same_as<T>;
        };

        template <DiscreteDistr T> uint8_t get_random() {
            constexpr int count = (T::max - T::min) / T::step + 1;
            static_assert(count == std::extent_v<decltype(T::weights)>, "weights array size mismatch, go fix");
            static std::discrete_distribution<uint8_t> dist(std::begin(T::weights), std::end(T::weights));

            return T::min + dist(rng::get()) * T::step;
        }
    }

    template <typename T>
    concept IsPowerUp = requires(PlayerStats& stats, T t) {
        { t.apply(stats) } -> std::same_as<void>;
        requires __impl::DiscreteDistr<T> || __impl::HasRandom<T>;
    };

    template <PowerUpType Type> struct Speed;
    template <> struct Speed<Percentage> {
        static constexpr const char* name = "Speed";
        static constexpr uint8_t min = 5;
        static constexpr uint8_t max = 35;
        static constexpr uint8_t step = 5;
        static constexpr double weights[] = {1, 2, 3, 4, 5, 6, 7};

        uint8_t value;

        void apply(PlayerStats& stats) {
            stats.speed.add_percentage(value);
        }
    };
    template <> struct Speed<Absolute> {
        static constexpr const char* name = "Speed";
        static constexpr uint8_t min = 1;
        static constexpr uint8_t max = 5;
        static constexpr uint8_t step = 1;
        static constexpr double weights[] = {1, 2, 3, 4, 5};

        uint8_t value;

        void apply(PlayerStats& stats) {
            stats.speed.add_points(value);
        }
    };

    template <PowerUpType Type> struct MaxHealth;
    template <> struct MaxHealth<Percentage> {
        static constexpr const char* name = "Max Health";
        static constexpr uint8_t min = 2;
        static constexpr uint8_t max = 16;
        static constexpr uint8_t step = 2;
        static constexpr double weights[] = {1, 2, 3, 4, 5, 6, 7, 8};

        uint8_t value;

        void apply(PlayerStats& stats) {
            stats.max_health.add_percentage(value);
        }
    };
    template <> struct MaxHealth<Absolute> {
        static constexpr const char* name = "Max Health";
        static constexpr uint8_t min = 5;
        static constexpr uint8_t max = 50;
        static constexpr uint8_t step = 5;
        static constexpr double weights[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        uint8_t value;

        void apply(PlayerStats& stats) {
            stats.max_health.add_points(value);
        }
    };

    template <PowerUpType Type> struct MaxMana;
    template <> struct MaxMana<Percentage> {
        static constexpr const char* name = "Max Mana";
        static constexpr uint8_t min = 2;
        static constexpr uint8_t max = 30;
        static constexpr uint8_t step = 4;
        static constexpr double weights[] = {1, 2, 3, 4, 5, 6, 7, 8};

        uint8_t value;

        void apply(PlayerStats& stats) {
            stats.max_mana.add_percentage(value);
        }
    };
    template <> struct MaxMana<Absolute> {
        static constexpr const char* name = "Max Mana";
        static constexpr uint8_t min = 30;
        static constexpr uint8_t max = 150;
        static constexpr uint8_t step = 10;
        static constexpr double weights[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

        uint8_t value;

        void apply(PlayerStats& stats) {
            stats.max_mana.add_points(value);
        }
    };

    template <PowerUpType Type> struct HealthRegen;
    template <> struct HealthRegen<Percentage> {
        static constexpr const char* name = "Health Regen";
        static constexpr uint8_t min = 2;
        static constexpr uint8_t max = 10;
        static constexpr uint8_t step = 1;
        static constexpr double weights[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

        uint8_t value;

        void apply(PlayerStats& stats) {
            stats.health_regen.add_percentage(value);
        }
    };
    template <> struct HealthRegen<Absolute> {
        static constexpr const char* name = "Health Regen";
        static constexpr uint8_t min = 1;
        static constexpr uint8_t max = 5;
        static constexpr uint8_t step = 1;
        static constexpr double weights[] = {1, 2, 3, 4, 5};

        uint8_t value;

        void apply(PlayerStats& stats) {
            stats.health_regen.add_points(value);
        }
    };

    template <PowerUpType Type> struct ManaRegen;
    template <> struct ManaRegen<Percentage> {
        static constexpr const char* name = "Mana Regen";
        static constexpr uint8_t min = 4;
        static constexpr uint8_t max = 16;
        static constexpr uint8_t step = 4;
        static constexpr double weights[] = {1, 2, 3, 4};

        uint8_t value;

        void apply(PlayerStats& stats) {
            stats.mana_regen.add_percentage(value);
        }
    };
    template <> struct ManaRegen<Absolute> {
        static constexpr const char* name = "Mana Regen";
        static constexpr uint8_t min = 2;
        static constexpr uint8_t max = 6;
        static constexpr uint8_t step = 1;
        static constexpr double weights[] = {1, 2, 3, 4, 5};

        uint8_t value;

        void apply(PlayerStats& stats) {
            stats.mana_regen.add_points(value);
        }
    };

    template <IsPowerUp P> P make_random() {
        if constexpr (__impl::HasRandom<P>) {
            return P::random();
        } else if constexpr (__impl::DiscreteDistr<P>) {
            P powerup;
            powerup.value = __impl::get_random<P>();

            return powerup;
        } else {
            static_assert(false, "How can this happen, fix your IsPowerUp concept");
        }
    }

#define EACH_POWERUP(F, G)                                                                                             \
    G(Speed)                                                                                                           \
    F(MaxHealth)                                                                                                       \
    F(MaxMana)                                                                                                         \
    F(HealthRegen)                                                                                                     \
    F(ManaRegen)

#define POWERUP_VARIANT_FIRST(name) name<Percentage>, name<Absolute>
#define POWERUP_VARIANT(name) , name<Percentage>, name<Absolute>
    using Data = std::variant<EACH_POWERUP(POWERUP_VARIANT, POWERUP_VARIANT_FIRST)>;
#undef POWERUP_VARIANT_FIRST
#undef POWERUP_VARIANT

#define POWERUP_TYPE_FIRST(name) name##Percentage = 0, name##Absolute,
#define POWERUP_TYPE(name) name##Percentage, name##Absolute,
    enum struct Type {
        EACH_POWERUP(POWERUP_TYPE, POWERUP_TYPE_FIRST) Size,
    };
#undef POWERUP_TYPE
#undef POWERUP_TYPE_FIRST

    template <Type T> struct EnumToType;

#define POWERUP_ENUM(name)                                                                                             \
    template <> struct EnumToType<Type::name##Percentage> {                                                            \
        using type = name<Percentage>;                                                                                 \
    };                                                                                                                 \
    template <> struct EnumToType<Type::name##Absolute> {                                                              \
        using type = name<Absolute>;                                                                                   \
    };

    EACH_POWERUP(POWERUP_ENUM, POWERUP_ENUM)
#undef POWERUP_ENUM

    template <IsPowerUp P> struct TypeToEnum;

#define POWERUP_TYPE_ENUM(name)                                                                                        \
    template <> struct TypeToEnum<name<Percentage>> {                                                                  \
        static constexpr Type type = Type::name##Percentage;                                                           \
    };                                                                                                                 \
    template <> struct TypeToEnum<name<Absolute>> {                                                                    \
        static constexpr Type type = Type::name##Absolute;                                                             \
    };

    EACH_POWERUP(POWERUP_TYPE_ENUM, POWERUP_TYPE_ENUM)
#undef POWERUP_TYPE_ENUM

    inline Data make_random(const Type& t) {
        switch (t) {
#define POWERUP_CASE(name)                                                                                             \
    case Type::name##Percentage:                                                                                       \
        return make_random<name<Percentage>>();                                                                        \
    case Type::name##Absolute:                                                                                         \
        return make_random<name<Absolute>>();

            EACH_POWERUP(POWERUP_CASE, POWERUP_CASE)
#undef POWERUP_CASE
            case Type::Size:
                assert(false && "Screw you");
        }

        std::unreachable();
    };
#undef EACH_POWERUP
}

struct PowerUp {
    powerups::Data power_up;

    PowerUp(powerups::Data&& data) : power_up(std::forward<powerups::Data>(data)) {
    }
    PowerUp(PowerUp&&) noexcept = default;
    PowerUp& operator=(PowerUp&&) noexcept = default;

    void draw(assets::Store& assets, const Rectangle& inside);
    void draw_hover(assets::Store& assets, const Rectangle& inside);
    void apply(PlayerStats& stats);
    static PowerUp random();
};
