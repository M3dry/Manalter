#include "enemies_spawner.hpp"

#include "enemies.hpp"
#include "hitbox.hpp"
#include "rayhacks.hpp"

bool Enemies::spawn(const Vector2& player_pos) {
    if (max_cap <= cap) return false;

    auto cap_diff = max_cap - cap;

    auto enemy = enemies::random_enemy(cap_diff, cap);
    if (!enemy.has_value()) return false;

    auto enemy_pos = (Vector2){ (float)GetRandomValue(0, 1000), (float)GetRandomValue(0, 1000) };
    auto player_radious_shape = shapes::Circle(player_pos, player_radious);

    while (check_collision(player_radious_shape, enemy_pos)) {
        enemy_pos = (Vector2){ (float)GetRandomValue(0, 1000), (float)GetRandomValue(0, 1000) };
    };

    enemies.emplace_back(enemy_pos, killed/100, false, std::move(enemy.value()));
    return true;
}

void Enemies::update_target(const Vector2& pos) {
    if (pos == target_pos) {
        return;
    }

    for (auto& enemy : enemies) {
        enemy.update_target(pos);
    }

    target_pos = pos;
}

uint32_t Enemies::tick(const shapes::Circle& target_hitbox, EnemyModels& enemy_models) {
    static uint8_t tick_count = 0;

    uint32_t acc = 0;
    for (auto& enemy : enemies) {
        acc += enemy.tick(target_hitbox, enemy_models);
    }

    if (++tick_count == 20) {
        spawn(target_pos);
        tick_count = 0;
    }

    return acc;
}

void Enemies::draw(EnemyModels& enemy_models, const Vector3& offset) const {
    for (const auto& enemy : enemies) {
        enemy.draw(enemy_models, offset);
    }
}
