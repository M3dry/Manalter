#include "enemies_spawner.hpp"

#include "enemies.hpp"
#include "hitbox.hpp"
#include "rayhacks.hpp"
#include "utility.hpp"
#include <raymath.h>

bool Enemies::spawn(const Vector2& player_pos) {
    static constexpr float arena_width = ARENA_WIDTH / 2.0f;
    static constexpr float arena_height = ARENA_HEIGHT / 2.0f;

    if (max_cap <= cap) return false;

    auto cap_diff = max_cap - cap;

    auto enemy = enemies::random_enemy(cap_diff, cap);
    if (!enemy.has_value()) return false;

    auto enemy_pos =
        (Vector2){(float)GetRandomValue(-arena_width, arena_width), (float)GetRandomValue(-arena_height, arena_height)};
    auto player_radious_shape = shapes::Circle(player_pos, player_radious);

    while (check_collision(player_radious_shape, enemy_pos)) {
        enemy_pos = (Vector2){(float)GetRandomValue(-arena_width, arena_width),
                              (float)GetRandomValue(-arena_height, arena_height)};
    };

    enemies.insert(Enemy(enemy_pos, killed / 100, false, std::move(enemy.value())));
    return true;
}

void Enemies::update_target(const Vector2& pos) {
    if (pos == target_pos) {
        return;
    }

    for (auto& enemy : enemies.data) {
        enemy.val.update_target(pos);
    }

    target_pos = pos;
}

uint32_t Enemies::tick(const shapes::Circle& target_hitbox, EnemyModels& enemy_models, const Vector2& new_target) {
    static uint8_t tick_count = 0;

    uint32_t acc = 0;
    for (auto& enemy : enemies.data) {
        acc += enemy.val.tick(target_hitbox, enemy_models,
                              target_pos == new_target ? std::nullopt : std::optional(target_pos));
    }

    enemies.rebuild();
    enemies.resolve_collisions([](const auto& e1, const auto& e2) -> std::optional<Vector2> {
        auto dist_sqr = Vector2DistanceSqr(e1.position(), e2.position());

#define SQR(x) ((x)*(x))
        if (dist_sqr <= SQR(e1.simple_hitbox.radius + e2.simple_hitbox.radius)) {
            if (dist_sqr == 0) {
                return Vector2Scale({ 1, 0 }, e1.simple_hitbox.radius);
            }

            auto collision_normal = Vector2Scale((e1.position() - e2.position()), std::sqrt(dist_sqr));
            return collision_normal * ((e1.simple_hitbox.radius + e2.simple_hitbox.radius) - std::sqrt(dist_sqr));
        }
#undef SQR

        return std::nullopt;
    }, 5);

    target_pos = new_target;

    if (++tick_count == 20) {
        spawn(target_pos);
        tick_count = 0;
    }

    return acc;
}

void Enemies::draw(EnemyModels& enemy_models, const Vector3& offset, const shapes::Circle& visibility_circle) const {
    for (const auto& enemy : enemies.data) {
        if (check_collision(visibility_circle, xz_component(Vector3Add(enemy.val.pos, offset))))
            enemy.val.draw(enemy_models, offset);
    }
}

uint32_t Enemies::take_exp() {
    auto exp = stored_exp;
    stored_exp = 0;
    return exp;
}
