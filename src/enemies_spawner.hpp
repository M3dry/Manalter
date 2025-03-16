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
    static constexpr float player_radious = 20.0f;

    uint32_t max_cap;
    uint32_t cap;
    quadtree::QuadTree<10, Enemy> enemies;
    uint64_t killed;
    Vector2 target_pos;

    Enemies(uint32_t max_cap) : max_cap(max_cap), cap(0), enemies(arena::arena_rec), target_pos(Vector2Zero()) {
    }

    // true - enemy spawned
    // false - cap is maxed out
    bool spawn(const Vector2& player_pos);

    void update_target(const Vector2& player_pos);

    template <Shape S>
    void deal_damage(S shape, uint32_t damage, Element element, std::vector<ItemDrop>& item_drop_pusher) {
        enemies.search_by(
            [&shape](const quadtree::Box& bbox) -> bool {
                shapes::Polygon rec = (Rectangle)bbox;

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
                if (!enemy.take_damage(damage, element)) return;

                item_drop_pusher.emplace_back(enemy.level, xz_component(enemy.pos));
                if (auto enemy_cap = enemies::get_info(enemy.state).cap_value; enemy_cap < cap) {
                    cap -= enemy_cap;
                } else {
                    cap = 0;
                }

                killed++;

                enemies.remove(ix);
            });
    }

    std::optional<std::size_t> closest_to(const Vector2& point) const;

    uint32_t tick(const shapes::Circle& target_hitbox, EnemyModels& enemy_models, const Vector2& target_pos);

    // TODO: deferred drawing
    void draw(EnemyModels& enemy_models, const Vector3& offset, const shapes::Circle& visibility_circle) const;
};
