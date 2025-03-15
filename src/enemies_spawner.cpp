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

std::optional<std::reference_wrapper<const Enemy>> Enemies::closest_to(const Vector2& point) const {
    /*if (enemies.size() == 0) return std::nullopt;*/
    /**/
    /*std::size_t closest = 0;*/
    /*float sqr_dist = Vector2DistanceSqr(point, xz_component(enemies[closest].position));*/
    /**/
    /*if (sqr_dist == 0) return enemies[closest];*/
    /**/
    /*for (std::size_t i = 1; i < enemies.size(); i++) {*/
    /*    if (auto tmp = Vector2DistanceSqr(point, xz_component(enemies[i].position)); tmp < sqr_dist) {*/
    /*        sqr_dist = tmp;*/
    /*        closest = i;*/
    /**/
    /*        if (sqr_dist == 0) break;*/
    /*    }*/
    /*}*/
    /**/
    /*return enemies[closest];*/
}

uint32_t Enemies::tick(const shapes::Circle& target_hitbox, EnemyModels& enemy_models) {
    static uint8_t tick_count = 0;

    uint32_t acc = 0;
    for (auto& enemy : enemies.data) {
        acc += enemy.val.tick(target_hitbox, enemy_models);
    }

    if (++tick_count == 20) {
        spawn(target_pos);
        tick_count = 0;
    }

    return acc;
}

void Enemies::draw(EnemyModels& enemy_models, const Vector3& offset, const shapes::Circle& visibility_circle) const {
    for (const auto& enemy : enemies.data) {
        if (check_collision(visibility_circle, xz_component(Vector3Add(enemy.val.pos, offset)))) enemy.val.draw(enemy_models, offset);
    }
}
