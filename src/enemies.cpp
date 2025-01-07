#include "enemies.hpp"
#include "rayhacks.hpp"
#include "raymath.h"
#include <cassert>

const std::array<float, static_cast<int>(EnemyType::Size)> Enemy::y_component = {

};

const std::array<uint8_t, static_cast<int>(EnemyType::Size)> Enemy::cap_value = {

};

const std::array<Element, static_cast<int>(EnemyType::Size)> Enemy::element = {

};

const std::array<std::pair<uint32_t, uint32_t>, static_cast<int>(EnemyType::Size)> damage_range = {

};

const std::array<std::pair<uint32_t, uint32_t>, static_cast<int>(EnemyType::Size)> speed_range = {

};

const std::array<uint32_t, static_cast<int>(EnemyType::Size)> Enemy::max_health = {

};

const std::array<float, static_cast<int>(EnemyType::Size)> Enemy::simple_hitbox_radius = {

};

Enemy::Enemy(EnemyType type, Vector2 target, Vector2 position)
    : position((Vector3){position.x, Enemy::y_component[static_cast<int>(type)], position.y}),
      movement(Vector2Normalize(position - target)),
      simple_hitbox(shapes::Circle(position, Enemy::simple_hitbox_radius[static_cast<int>(type)])), type(type) {
    // TODO: health, damage, speed,
    //       angle, state
}

shapes::Polygon Enemy::hitbox(EnemyType type) const {
    assert(type != EnemyType::Size);

    std::unreachable();
}

void Enemy::update_target(Vector2 new_target) {
    movement = Vector2Normalize((Vector2){position.x, position.z} - new_target);
    // TODO: Reset enemy state?
}

void Enemy::tick(shapes::Polygon target_hitbox) {
    position.x += movement.x;
    position.y += movement.y;

    // TODO: change state
}
