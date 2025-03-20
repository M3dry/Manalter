#pragma once

#include <concepts>
#include <cstdint>
#include <random>
#include <variant>

#include "player.hpp"

struct Player;

enum struct PowerUpType {
    Percentage,
    Absolute,
};

template <typename T>
concept IsPowerUp = requires(Player& player) {
    &T::apply(player);
    &T::apply(player);
};

namespace powerups {
    using enum PowerUpType;

    namespace __impl {
        template <typename T>
        concept DiscreteDistr = requires() {
            { T::min } -> std::convertible_to<uint8_t>;
            { T::max } -> std::convertible_to<uint8_t>;
            { T::step } -> std::convertible_to<uint8_t>;
            { T::weights } -> std::same_as<const double(&)[std::extent_v<decltype(T::weights)>]>;
        };

        template <DiscreteDistr T> uint8_t get_random() {
            constexpr int count = (T::max - T::min) / T::step + 1;
            static_assert(count == std::extent_v<decltype(T::weights)>, "weights array size mismatch, go fix");
            static std::discrete_distribution<int> dist(std::begin(T::weights), std::end(T::weights));

            return T::min + dist(rng::get()) * T::step;
        }
    }

    template <PowerUpType Type> struct Speed;
    template <> struct Speed<Percentage> {
        static constexpr uint8_t min = 5;
        static constexpr uint8_t max = 35;
        static constexpr uint8_t step = 5;
        static constexpr double weights[] = {1, 2, 3, 4, 5, 6, 7};
    };
    template <> struct Speed<Absolute> {
        static constexpr uint8_t min = 1;
        static constexpr uint8_t max = 5;
        static constexpr uint8_t step = 1;
        static constexpr double weights[] = {1, 2, 3, 4, 5};
    };

    template <PowerUpType Type> struct Damage;
    template <> struct Damage<Percentage> {
        static constexpr uint8_t min = 5;
        static constexpr uint8_t max = 75;
        static constexpr uint8_t step = 5;
        static constexpr double weights[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    };
    template <> struct Damage<Absolute> {
        static constexpr uint8_t min = 2;
        static constexpr uint8_t max = 10;
        static constexpr uint8_t step = 1;
        static constexpr double weights[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    };

    template <PowerUpType Type> struct MaxHealth;
    template <> struct MaxHealth<Percentage> {
        static constexpr uint8_t min = 2;
        static constexpr uint8_t max = 16;
        static constexpr uint8_t step = 2;
        static constexpr double weights[] = {1, 2, 3, 4, 5, 6, 7, 8};
    };
    template <> struct MaxHealth<Absolute> {
        static constexpr uint8_t min = 5;
        static constexpr uint8_t max = 50;
        static constexpr uint8_t step = 5;
        static constexpr double weights[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    };

    template <PowerUpType Type> struct HealthRegen;
    template <> struct HealthRegen<Percentage> {
        static constexpr uint8_t min = 2;
        static constexpr uint8_t max = 10;
        static constexpr uint8_t step = 1;
        static constexpr double weights[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    };
    template <> struct HealthRegen<Absolute> {
        static constexpr uint8_t min = 1;
        static constexpr uint8_t max = 5;
        static constexpr uint8_t step = 1;
        static constexpr double weights[] = {1, 2, 3, 4, 5};
    };

    template <PowerUpType Type> struct MaxMana;
    template <> struct MaxMana<Percentage> {
        static constexpr uint8_t min = 2;
        static constexpr uint8_t max = 30;
        static constexpr uint8_t step = 4;
        static constexpr double weights[] = {1, 2, 3, 4, 5, 6, 7, 8};
    };
    template <> struct MaxMana<Absolute> {
        static constexpr uint8_t min = 30;
        static constexpr uint8_t max = 150;
        static constexpr uint8_t step = 10;
        static constexpr double weights[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
    };

    template <PowerUpType Type> struct ManaRegen;
    template <> struct ManaRegen<Percentage> {
        static constexpr uint8_t min = 4;
        static constexpr uint8_t max = 16;
        static constexpr uint8_t step = 4;
        static constexpr double weights[] = {1, 2, 3, 4};
    };
    template <> struct ManaRegen<Absolute> {
        static constexpr uint8_t min = 2;
        static constexpr uint8_t max = 6;
        static constexpr uint8_t step = 1;
        static constexpr double weights[] = {1, 2, 3, 4, 5};
    };

#define EACH_POWERUP(F, G)                                                                                             \
    G(Speed<Percentage>)                                                                                               \
    F(Speed<Absolute>)                                                                                                 \
    F(Damage<Percentage>)                                                                                              \
    F(Damage<Absolute>)                                                                                                \
    F(MaxHealth<Percentage>)                                                                                           \
    F(MaxHealth<Absolute>)                                                                                             \
    F(HealthRegen<Percentage>)                                                                                         \
    F(HealthRegen<Absolute>)                                                                                           \
    F(MaxMana<Percentage>)                                                                                             \
    F(MaxMana<Absolute>)                                                                                               \
    F(ManaRegen<Percentage>)                                                                                           \
    F(ManaRegen<Absolute>)

#define POWERUP_VARIANT_FIRST(name) name
#define POWERUP_VARIANT(name) , name
    using Variant = std::variant<EACH_POWERUP(POWERUP_VARIANT, POWERUP_VARIANT_FIRST)>;
#undef POWERUP_VARIANT_FIRST
#undef POWERUP_VARIANT

#undef EACH_POWERUP
}

struct PowerUp {};
