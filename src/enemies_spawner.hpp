#pragma once

#include "enemies.hpp"
#include "hitbox.hpp"
#include "item_drops.hpp"
#include "utility.hpp"
#include <cstdint>
#include <vector>

struct Enemies {
    // enemies can't spawn inside this circle, centered at player
    static constexpr float player_radious = 20.0f;

    uint32_t max_cap;
    uint32_t cap;
    std::vector<Enemy> enemies;
    uint64_t killed;
    Vector2 target_pos;

    Enemies(uint32_t max_cap) : max_cap(max_cap), cap(0), enemies(), target_pos(Vector2Zero()) {
    }

    // true - enemy spawned
    // false - cap is maxed out
    bool spawn(const Vector2& player_pos);

    void update_target(const Vector2& player_pos);

    void deal_damage(const Shape auto& shape, uint32_t damage, Element element,
                     std::vector<ItemDrop>& item_drop_pusher) {
        auto [first, last] = std::ranges::remove_if(enemies, [&](auto& enemy) -> bool {
            bool dead = false;

            if (check_collision(shape, enemy.simple_hitbox)) {
                dead = enemy.take_damage(damage, element);
            }

            if (dead) {
                item_drop_pusher.emplace_back(enemy.level, xz_component(enemy.position));
                if (auto enemy_cap = enemies::get_info(enemy.state).cap_value; enemy_cap < cap) {
                    cap -= enemy_cap;
                } else {
                    cap = 0;
                }
                killed++;
            }

            return dead;
        });
        enemies.erase(first, last);
    }

    std::optional<std::reference_wrapper<const Enemy>> closest_to(const Vector2& point) const;

    // TODO: use modules, else I'll fucking kms
    uint32_t tick(const shapes::Circle& target_hitbox, EnemyModels& enemy_models);

    // TODO: deferred drawing
    void draw(EnemyModels& enemy_models, const Vector3& offset, const shapes::Circle& visibility_circle) const;
};
