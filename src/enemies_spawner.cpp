#include "enemies_spawner.hpp"

#include "enemies.hpp"
#include "hitbox.hpp"
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

    int base_level = std::ceil(killed / 100.0f);
    auto lvl = GetRandomValue(std::max(base_level - 2, 1), base_level + 2);
    auto _enemy = Enemy(enemy_pos, lvl, false, std::move(enemy.value()));
    enemies.insert(std::move(_enemy));
    return true;
}

uint32_t Enemies::tick(const shapes::Circle& target_hitbox, EnemyModels& enemy_models) {
    static uint8_t tick_count = 0;

    uint32_t acc = 0;
    for (std::size_t i = 0; i < enemies.data.size(); i++) {
        acc += enemies.data[i].val.tick(enemies, i, target_hitbox, enemy_models);
    }

    enemies.rebuild();

    if (++tick_count == 20) {
        spawn(target_hitbox.center);
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
    if (exp != 0) std::println("EXP TAKEN: {}", exp);
    stored_exp = 0;
    return exp;
}
