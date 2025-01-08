#pragma once

#include "hitbox.hpp"
#include "raylib.h"
#include "spell.hpp"
#include <array>
#include <cstdint>
#include <variant>

#define BOSS_SCALE_FACTOR 10.0f

enum struct EnemyType {
    Size,
};

struct Enemy {
    static const std::array<float, static_cast<int>(EnemyType::Size)> y_component;
    static const std::array<uint8_t, static_cast<int>(EnemyType::Size)> cap_value;
    static const std::array<Element, static_cast<int>(EnemyType::Size)> element;
    static const std::array<std::pair<uint32_t, uint32_t>, static_cast<int>(EnemyType::Size)> damage_range;
    static const std::array<std::pair<uint32_t, uint32_t>, static_cast<int>(EnemyType::Size)> speed_range;
    static const std::array<uint32_t, static_cast<int>(EnemyType::Size)> max_health;
    static const std::array<float, static_cast<int>(EnemyType::Size)> simple_hitbox_radius;

    uint32_t health;
    uint32_t damage;
    uint16_t speed;

    Vector3 position;
    Vector2 movement;
    float angle;
    // for proper collisions also check against `hitbox(EnemyType)` if `simple_hitbox` detects a collision
    shapes::Circle simple_hitbox;

    EnemyType type;
    // Stats are multiplied by `BOSS_SCALE_FACTOR` and model is increased by 2x
    bool boss;
    // TODO: State for each enemy type
    std::variant<int> state;

    Enemy(EnemyType type, Vector2 target, Vector2 position, bool boss);

    // only use if `simple_hitbox` determined a collision
    shapes::Polygon hitbox(EnemyType type) const;
    void update_target(Vector2 new_target);
    void tick(shapes::Polygon target_hitbox);
};
