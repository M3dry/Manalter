#include "enemies_spawner.hpp"

#include "enemies.hpp"
#include "hitbox.hpp"
#include "rayhacks.hpp"
#include "utility.hpp"

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

std::optional<ItemDrop> Enemies::deal_damage(std::size_t enemy, uint32_t damage, Element element) {
    // TODO: Elemental damage scaling
    if (enemies[enemy].health < damage) {
        enemies[enemy].health = 0;
    } else {
        enemies[enemy].health -= damage;
    }

    if (enemies[enemy].health <= 0) {
        auto level = enemies[enemy].level;
        auto pos = enemies[enemy].position;

        // TODO: should probably do something more efficient here
        enemies.erase(enemies.begin() + enemy);

        killed++;
        return ItemDrop::random(level, xz_component(pos));
    }

    return std::nullopt;
}

uint32_t Enemies::tick(const shapes::Circle& target_hitbox) {
    static uint8_t tick_count = 0;

    uint32_t acc = 0;
    for (auto& enemy : enemies) {
        acc += enemy.tick(target_hitbox);
    }

    if (++tick_count == 20) {
        spawn(target_pos);
        tick_count = 0;
    }

    return acc;
}

void Enemies::draw() const {
    for (const auto& enemy : enemies) {
        enemy.draw();
    }
}
