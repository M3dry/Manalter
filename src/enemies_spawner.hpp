#pragma once

#include "enemies.hpp"
#include "hitbox.hpp"
#include "item_drops.hpp"
#include "quadtree.hpp"
#include "rayhacks.hpp"
#include "utility.hpp"
#include <cstdint>
#include <vector>

struct Enemies {
    // enemies can't spawn inside this circle, centered at player
    static constexpr float player_radious = 50.0f;

    uint32_t max_cap;
    uint32_t cap;
    QT<false> enemies;
    uint64_t killed = 0;
    uint32_t stored_exp = 0;
    uint64_t stored_souls = 0;

    Enemies(uint32_t max_cap) : max_cap(max_cap), cap(0), enemies(arena::arena_rec) {
    }

    // true - enemy spawned
    // false - cap is maxed out
    bool spawn(const EnemyModels& enemy_models, const Vector2& player_pos);

    template <Shape S>
    uint32_t deal_damage(S shape, uint64_t damage, Element element, std::vector<ItemDrop>& item_drop_pusher) {
        uint32_t spell_exp = 0;
        enemies.search_by(
            [&shape](const quadtree::Box& bbox) -> bool {
                shapes::Polygon rec = static_cast<Rectangle>(bbox);

                for (const auto& origin : {Vector2Zero(), arena::top_origin, arena::right_origin, arena::bottom_origin,
                                           arena::left_origin}) {
                    shape.translate(origin);
                    if (check_collision(rec, shape)) {
                        return true;
                        shape.translate(-origin);
                    }
                    shape.translate(-origin);
                }

                return false;
            },
            [&](const auto& enemy) {
                auto& hitbox = enemy.simple_hitbox;

                for (const auto& origin : {Vector2Zero(), arena::top_origin, arena::right_origin, arena::bottom_origin,
                                           arena::left_origin}) {
                    shape.translate(origin);
                    if (check_collision(shape, hitbox)) {
                        return true;
                        shape.translate(-origin);
                    }
                    shape.translate(-origin);
                }

                return false;
            },
            [&](auto& enemy, auto ix) {
                auto dead = enemy.take_damage(damage, element);
                if (!dead) return;
                auto [exp, souls] = *dead;

                if (GetRandomValue(0, 5) == 0) {
                    item_drop_pusher.emplace_back(enemy.level, xz_component(enemy.pos));
                }

                if (auto enemy_cap = enemies::get_info(enemy.state).cap_value; enemy_cap < cap) {
                    cap -= enemy_cap;
                } else {
                    cap = 0;
                }
                cap = std::min<uint32_t>(cap + 1, 500);

                killed++;
                stored_exp += exp;
                spell_exp += exp;
                stored_souls += souls;
                enemies.remove(ix);
            });

        return spell_exp;
    }

    uint32_t tick(const shapes::Circle& target_hitbox, EnemyModels& enemy_models);

    void draw(EnemyModels& enemy_models, const Vector3& offset, const shapes::Circle& visibility_circle);

    uint32_t take_exp();
    uint64_t take_souls();
};
