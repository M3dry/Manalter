#pragma once

#include "enemies.hpp"
#include <cstdint>
#include <vector>
#include "hitbox.hpp"
#include "item_drops.hpp"

// TODO: change name to Enemies and stop using enemies.hpp::Enemies
struct Enemies {
    // enemies can't spawn inside this circle, centered at player
    static constexpr float player_radious = 20.0f;

    uint32_t max_cap;
    uint32_t cap;
    std::vector<Enemy> enemies;
    uint64_t killed;
    Vector2 target_pos;

    Enemies(uint32_t max_cap) : max_cap(max_cap), cap(0), enemies(), target_pos(Vector2Zero()) {}

    // true - enemy spawned
    // false - cap is maxed out
    bool spawn(const Vector2& player_pos);

    void update_target(const Vector2& player_pos);

    // std::null_opt - no collision detected
    // n - index of enemy that was first detected as a collision
    std::optional<std::size_t> collides_with_any(const Shape auto& shape) const {
        for (std::size_t i = 0; i < enemies.size(); i++) {
            if (check_collision(enemies[i].simple_hitbox, shape)) {
                return i;
            }
        }

        return std::nullopt;
    }

    // indexes of all enemies for which a collision was detected
    std::vector<std::size_t> all_collisions(const Shape auto& shape) const {
        std::vector<std::size_t> vec;

        for (std::size_t i = 0; i < enemies.size(); i++) {
            if (check_collision(enemies[i].simple_hitbox, shape)) {
                vec.emplace_back(i);
            }
        }

        return vec;
    }

    std::optional<ItemDrop> deal_damage(std::size_t enemy, uint32_t damage, Element element);

    // TODO: use modules, else I'll fucking kms
    uint32_t tick(const shapes::Circle& target_hitbox);

    // TODO: deferred drawing
    void draw() const;
};
