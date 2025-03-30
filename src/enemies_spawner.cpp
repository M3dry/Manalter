#include "enemies_spawner.hpp"

#include "enemies.hpp"
#include "hitbox.hpp"
#include "player.hpp"
#include "utility.hpp"
#include <random>
#include <raylib.h>
#include <raymath.h>

bool Enemies::spawn(const EnemyModels& enemy_models, const Vector2& player_pos) {
    static constexpr float arena_width = ARENA_WIDTH / 2.0f;
    static constexpr float arena_height = ARENA_HEIGHT / 2.0f;
    static std::uniform_real_distribution<float> radiusDist(Player::visibility_radius - 2 * player_radious,
                                                            Player::visibility_radius);
    static std::uniform_real_distribution<float> angleDist(0, 2.0f * std::numbers::pi);

    if (max_cap <= cap) return false;

    auto cap_diff = max_cap - cap;

    auto enemy = enemies::random_enemy(cap_diff, cap);
    if (!enemy.has_value()) return false;

    auto radius = radiusDist(rng::get());
    auto angle = angleDist(rng::get());
    auto enemy_pos = Vector2{
        .x = radius * std::cos(angle) + player_pos.x,
        .y = radius * std::sin(angle) + player_pos.y,
    };

    int base_level = std::ceil(killed / 100.0f);
    auto lvl = GetRandomValue(std::max(base_level - 2, 1), base_level + 2);

    auto bone_data = enemy_models.get_bone_transforms(*enemy);
    auto _enemy = Enemy(enemy_pos, lvl, false, std::move(enemy.value()), std::move(bone_data));
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
        spawn(enemy_models, target_hitbox.center);
        tick_count = 0;
    }

    return acc;
}

void Enemies::draw(EnemyModels& enemy_models, const Vector3& offset, const shapes::Circle& visibility_circle) {
    std::vector<Matrix> instance_transforms;
    std::vector<Matrix> bone_transforms;
    for (auto& enemy : enemies.data) {
        if (check_collision(visibility_circle, xz_component(Vector3Add(enemy.val.pos, offset)))) {
            enemy.val.update_bones(enemy_models);
            enemy.val.draw(enemy_models, offset);
        }
    }
}

uint32_t Enemies::take_exp() {
    auto exp = stored_exp;
    stored_exp = 0;
    return exp;
}
