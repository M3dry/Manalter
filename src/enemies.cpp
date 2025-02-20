#include "enemies.hpp"
#include "hitbox.hpp"
#include "rayhacks.hpp"
#include "raylib.h"
#include "raymath.h"
#include "utility.hpp"
#include <cassert>

namespace enemies {
    int Paladin::tick(Enemy& data, const shapes::Circle target_hitbox) {
        switch (data.collision_state) {
            case Enemy::Collision:
                data.anim_index = 0;
                data.anim_curr_frame = 0;
                return data.damage;
            case Enemy::Uncollision:
                data.anim_index = 1;
                data.anim_curr_frame = 0;
                return 0;
            case Enemy::Unchanged:
                return 0;
        }
    }

    int Zombie::tick(Enemy& data, const shapes::Circle target_hitbox) {
        return 0;
    }

    int Heraklios::tick(Enemy& data, const shapes::Circle target_hitbox) {
        return 0;
    }

    int Maw::tick(Enemy& data, const shapes::Circle target_hitbox) {
        return 0;
    }

    std::optional<State> random_enemy(uint32_t available_cap, uint32_t& cap) {
        int nth = GetRandomValue(1, static_cast<int>(_EnemyType::Size));
        std::size_t i = 0, len = infos.size();
        bool exists_in_cap = false;

        while (nth != 0) {
            if (infos[i].cap_value <= available_cap) {
                exists_in_cap = true;
                nth--;
                if (nth == 0) {
                    continue;
                }
            }

            if (++i >= len) {
                if (!exists_in_cap) return std::nullopt;
                i = 0;
            }
        }

        return create_enemy(static_cast<_EnemyType>(i));
    }

    Info get_info(const State& state) {
        return std::visit([](auto&& arg) { return std::decay_t<decltype(arg)>::info; }, state);
    }

    _EnemyType get_type(const State& state) {
        return std::visit(
            [](auto&& arg) { return EnumFromEnemy<std::remove_const_t<std::decay_t<decltype(arg)>>>::type; }, state);
    }
}

EnemyModels::EnemyModels() {
    for (int i = 0; i < static_cast<int>(enemies::_EnemyType::Size); i++) {
        auto type = static_cast<enemies::_EnemyType>(i);
        auto model_path = get_info(type).model_path;

        Animation anim;
        anim.animations = LoadModelAnimations(model_path, &anim.count);

        auto model = LoadModel(model_path);
        model.transform = MatrixMultiply(model.transform, MatrixRotateX(std::numbers::pi / 2.0f));

        models[i] = { model, anim };
    }
}

std::pair<Model, EnemyModels::Animation> EnemyModels::operator[](const enemies::State& state) {
    return models[static_cast<int>(enemies::get_type(state))];
}

EnemyModels::~EnemyModels() {
    for (auto& [model, animation] : models) {
        UnloadModel(model);
        UnloadModelAnimations(animation.animations, animation.count);
    }
}

void Enemy::draw(EnemyModels& enemy_models) const {
    auto [model, animation] = enemy_models[state];
    UpdateModelAnimation(model, animation.animations[anim_index], anim_curr_frame);
    DrawModelEx(model, position, (Vector3){0.0f, 1.0f, 0.0f}, angle,
                std::visit(
                    [](auto&& arg) -> Vector3 {
                        auto scale = std::decay_t<decltype(arg)>::info.model_scale;
                        return (Vector3){scale, scale, scale};
                    },
                    state),
                WHITE);
#ifdef DEBUG
    simple_hitbox.draw_3D(RED, 1.0f);
#endif
}

void Enemy::update_target(Vector2 new_target) {
    movement = Vector2Normalize(new_target - (Vector2){position.x, position.z});
    angle = angle_from_point(new_target, (Vector2){position.x, position.z});
}

uint32_t Enemy::tick(shapes::Circle target_hitbox, EnemyModels& enemy_models) {
    return std::visit(
        [&](auto&& arg) {
            position.x += movement.x * speed;
            position.z += movement.y * speed;

            simple_hitbox.center.x = position.x;
            simple_hitbox.center.y = position.z;

            auto [_, animation] = enemy_models[state];
            anim_curr_frame = (anim_curr_frame + 3) % animation.animations[anim_index].frameCount;

            if (check_collision(target_hitbox, simple_hitbox)) {
                switch (collision_state) {
                    case Collision:
                        collision_state = Unchanged;
                        break;
                    default:
                        collision_state = Collision;
                }
            } else {
                switch (collision_state) {
                    case Uncollision:
                        collision_state = Unchanged;
                        break;
                    default:
                        collision_state = Uncollision;
                }
            }

            return arg.tick(*this, target_hitbox);
        },
        state);
}

bool Enemy::take_damage(uint32_t damage, Element element) {
    // TODO: Elemental damage scaling
    if (health <= damage) {
        health = 0;
        return true;
    }

    health -= damage;
    return false;
}
