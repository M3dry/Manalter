#include "enemies.hpp"
#include "hitbox.hpp"
#include "raylib.h"
#include "raymath.h"
#include "utility.hpp"
#include <cassert>
#include <memory>

namespace enemies {
    uint32_t Paladin::tick(QT<true>& enemies, std::size_t ix, const shapes::Circle target_hitbox) {
        auto& data = enemies.data[ix].val;

        switch (data.collision_state) {
            case Enemy::Collision:
                data.anim_index = 0;
                /*data.anim_curr_frame = 0;*/
                data.movement = Vector2Zero();
                return data.damage;
            case Enemy::Uncollision:
                data.anim_index = 1;
                /*data.anim_curr_frame = 0;*/
                data.update_target(enemies, target_hitbox.center, ix);
                return 0;
            case Enemy::Unchanged:
                return 0;
        }
    }

    uint32_t Zombie::tick([[maybe_unused]] QT<true>& enemies, [[maybe_unused]] std::size_t ix,
                     [[maybe_unused]] const shapes::Circle target_hitbox) {
        return 0;
    }

    uint32_t Heraklios::tick([[maybe_unused]] QT<true>& enemies, [[maybe_unused]] std::size_t ix,
                        [[maybe_unused]] const shapes::Circle target_hitbox) {
        return 0;
    }

    uint32_t Maw::tick([[maybe_unused]] QT<true>& enemies, [[maybe_unused]] std::size_t ix,
                  [[maybe_unused]] const shapes::Circle target_hitbox) {
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

        auto type = static_cast<_EnemyType>(i);
        cap += enemies::get_info(type).cap_value;
        return create_enemy(type);
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
    for (std::size_t i = 0; i < static_cast<int>(enemies::_EnemyType::Size); i++) {
        auto type = static_cast<enemies::_EnemyType>(i);
        auto model_path = get_info(type).model_path;

        Animation anim;
        anim.animations = LoadModelAnimations(model_path, &anim.count);

        auto model = LoadModel(model_path);
        model.transform = MatrixMultiply(model.transform, MatrixRotateX(static_cast<float>(std::numbers::pi) / 2.0f));

        models[i] = {model, anim};
    }
}

std::pair<Model, EnemyModels::Animation> EnemyModels::operator[](const enemies::State& state) const {
    return models[static_cast<std::size_t>(enemies::get_type(state))];
}

std::vector<Matrix> EnemyModels::get_bone_transforms(const enemies::State& state) const {
    auto [model, _] = (*this)[state];

    std::vector<Matrix> bones;
    for (int i = 0; i < model.meshCount; i++) {
        bones.insert(bones.end(), model.meshes[i].boneMatrices,
                     model.meshes[i].boneMatrices + model.meshes[i].boneCount);
    }

    return bones;
}

void EnemyModels::update_bones(const enemies::State& state, std::vector<Matrix>& bone_transforms, int anim_index,
                               int anim_frame) {
    auto [model, animation] = (*this)[state];

    auto mesh_bone_ptrs = std::unique_ptr<Matrix*[]>(new Matrix*[static_cast<unsigned int>(model.meshCount)]);
    int sum = 0;
    for (std::size_t i = 0; i < static_cast<std::size_t>(model.meshCount); i++) {
        mesh_bone_ptrs[i] = model.meshes[i].boneMatrices;

        model.meshes[i].boneMatrices = bone_transforms.data() + sum;
        sum += model.meshes[i].boneCount;
    }

    UpdateModelAnimationBones(model, animation.animations[anim_index], anim_frame);

    for (std::size_t i = 0; i < static_cast<std::size_t>(model.meshCount); i++) {
        model.meshes[i].boneMatrices = mesh_bone_ptrs[i];
    }
}

void EnemyModels::add_shader(Shader shader) {
    for (auto& [model, _] : models) {
        for (int i = 0; i < model.materialCount; i++) {
            model.materials[i].shader = shader;
        }
    }
}

EnemyModels::~EnemyModels() {
    for (auto& [model, animation] : models) {
        UnloadModel(model);
        UnloadModelAnimations(animation.animations, animation.count);
    }
}

void Enemy::update_bones(EnemyModels& enemy_models) {
    enemy_models.update_bones(state, bone_transforms, anim_index, anim_curr_frame);
}

void Enemy::draw(EnemyModels& enemy_models, const Vector3& offset) {
    auto [model, _] = enemy_models[state];

    auto mesh_bone_ptrs = std::unique_ptr<Matrix*[]>(new Matrix*[static_cast<unsigned int>(model.meshCount)]);
    int sum = 0;
    for (std::size_t i = 0; i < static_cast<std::size_t>(model.meshCount); i++) {
        mesh_bone_ptrs[i] = model.meshes[i].boneMatrices;

        model.meshes[i].boneMatrices = bone_transforms.data() + sum;
        sum += model.meshes[i].boneCount;
    }

    DrawModelEx(model, Vector3Add(pos, offset), (Vector3){0.0f, 1.0f, 0.0f}, angle,
                std::visit(
                    [](auto&& arg) -> Vector3 {
                        auto scale = std::decay_t<decltype(arg)>::info.model_scale;
                        return (Vector3){scale, scale, scale};
                    },
                    state),
                damage_tint_left == 0 ? WHITE : damage_tint);
#ifdef DEBUG
    simple_hitbox.draw_3D(RED, 1.0f, xz_component(offset));
#endif

    for (std::size_t i = 0; i < static_cast<std::size_t>(model.meshCount); i++) {
        model.meshes[i].boneMatrices = mesh_bone_ptrs[i];
    }
}

void Enemy::update_target(QT<true>& enemies, Vector2 player_pos, std::size_t ix) {
    static const float neighbourhood_radius = 100.0f;
    static constexpr float weight_attraction = 3.0f; // attraction to player
    static constexpr float weight_separation = 11.0f;

    auto id = enemies.data[ix].id;
    auto enemy_pos = enemies.data[ix].val.position();

    shapes::Circle circle_hitbox(enemy_pos, neighbourhood_radius);

    Vector2 attraction_force;
    attraction_force.x = player_pos.x - enemy_pos.x;
    attraction_force.y = player_pos.y - enemy_pos.y;
    attraction_force.x = wrap((attraction_force.x + ARENA_WIDTH / 2.0f), ARENA_WIDTH) - ARENA_WIDTH / 2.0f;
    attraction_force.y = wrap((attraction_force.y + ARENA_WIDTH / 2.0f), ARENA_WIDTH) - ARENA_WIDTH / 2.0f;
    if (Vector2LengthSqr(attraction_force) > 1e-6f) {
        attraction_force = Vector2Normalize(attraction_force);
    }
    angle = std::fmod(270 - std::atan2(-attraction_force.y, -attraction_force.x) * 180.0f / static_cast<float>(std::numbers::pi), 360.0f);

    Vector2 separation_force = Vector2Zero();

    // FIXME: I don't think this gets all the neighbouring enemies correctly as using the for loop the enemies don't go inside each other at all
    enemies.search_by(
        [&circle_hitbox](const auto& bbox) -> bool { return check_collision(static_cast<Rectangle>(bbox), circle_hitbox); },
        [&circle_hitbox](const auto& enemy) -> bool { return check_collision(enemy.simple_hitbox, circle_hitbox); },
        [&](const auto& e, auto e_ix) {
            if (e_ix == ix || enemies.data[e_ix].id == id) return;

            auto d = enemy_pos - e.position();

            separation_force += Vector2Scale(Vector2Normalize(d), 1.0f / std::max(Vector2Length(d), 1e-6f));
        });
    /*for (const auto& [e_id, enemy] : enemies.data) {*/
    /*    if (e_id == id) continue;*/
    /**/
    /*    auto d = enemy_pos - enemy.position();*/
    /*    separation_force += Vector2Scale(Vector2Normalize(d), 1.0f / std::max(Vector2Length(d), 1e-6f));*/
    /*}*/

    movement = attraction_force * weight_attraction + separation_force * weight_separation;
    movement +=
        Vector2Scale(Vector2Normalize({GetRandomValue(-100, 100) / 100.f, GetRandomValue(-100, 100) / 100.f}), 0.1f);
    movement = Vector2Normalize(movement);
}

uint32_t Enemy::tick(QT<true>& enemies, std::size_t ix, shapes::Circle target_hitbox, EnemyModels& enemy_models) {
    return std::visit(
        [&](auto&& arg) {
            if (damage_tint_left != 0) damage_tint_left--;

            set_position({movement.x * speed, movement.y * speed});
            update_target(enemies, target_hitbox.center, ix);

            auto [_, animation] = enemy_models[state];
            anim_curr_frame = (anim_curr_frame + 3) % animation.animations[anim_index].frameCount;

            // BUG: I don't think this is getting the right results, fix pwetty pwease
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

            return arg.tick(enemies, ix, target_hitbox);
        },
        state);
}

std::optional<uint32_t> Enemy::take_damage(uint64_t taken_damage, [[maybe_unused]] Element element) {
    // TODO: Elemental damage scaling
    if (health <= taken_damage) {
        health = 0;
        return dropped_exp();
    }

    damage_tint_left = damage_tint_init;

    health -= damage;
    return std::nullopt;
}

Vector2 Enemy::position() const {
    return xz_component(pos);
}

void Enemy::set_position(const Vector2& p) {
    pos.x += p.x;
    pos.z += p.y;
    arena::loop_around(pos.x, pos.z);

    simple_hitbox.center.x = pos.x;
    simple_hitbox.center.y = pos.z;
}

uint32_t Enemy::dropped_exp() const {
    auto info = enemies::get_info(state);

    return info.base_exp_dropped * level * (boss ? 10 : 1);
}
