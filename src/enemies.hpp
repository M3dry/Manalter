#pragma once

#include "hitbox.hpp"
#include "raylib.h"
#include "raymath.h"
#include "spell.hpp"
#include <cstdint>
#include <variant>

struct Enemy;

namespace enemies {
    enum struct EnemyType {
        Human,
        Undead,
        Mage,
        Giant,
    };

    struct Info {
        const char* model_path;
        float model_scale;
        int default_anim;
        float y_component;
        uint32_t cap_value;
        EnemyType element;
        std::pair<uint32_t, uint32_t> damage_range;
        std::pair<uint32_t, uint32_t> speed_range;
        uint32_t max_health;
        float simple_hitbox_radius;
    };

    struct Paladin {
        enum Animations {
            HeadButt = 0,
            Sprinting = 1,
        };

        int tick(Enemy& data, const shapes::Circle target_hitbox);

        static constexpr Info info = (Info){
            .model_path = "./assets/paladin.glb",
            .model_scale = 0.3f,
            .default_anim = 1,
            .y_component = 1.0f,
            .cap_value = 2,
            .element = EnemyType::Human,
            .damage_range = {15, 20},
            .speed_range = {2.5, 2.5},
            .max_health = 100,
            .simple_hitbox_radius = 10.0f,
        };
    };

    struct Zombie {
        int tick(Enemy& data, const shapes::Circle target_hitbox);

        static constexpr Info info = (Info){
            .model_path = "./assets/zombie.glb",
            .y_component = 1.0f,
            .cap_value = 1,
            .element = EnemyType::Undead,
            .damage_range = {10, 20},
            .speed_range = {1, 3},
            .max_health = 50,
            .simple_hitbox_radius = 5.0f,
        };
    };

    struct Heraklios {
        int tick(Enemy& data, const shapes::Circle target_hitbox);

        static constexpr Info info = (Info){
            .model_path = "./assets/heraklios.glb",
            .model_scale = 0.2f,
            .default_anim = 0,
            .y_component = 1.0f,
            .cap_value = 5,
            .element = EnemyType::Mage,
            .damage_range = {30, 50},
            .speed_range = {5, 10},
            .max_health = 300,
            .simple_hitbox_radius = 5.0f,
        };
    };

    struct Maw {
        int tick(Enemy& data, const shapes::Circle target_hitbox);

        static constexpr Info info = (Info){
            .model_path = "./assets/maw.glb",
            .model_scale = 0.2f,
            .default_anim = 0,
            .y_component = 1.0f,
            .cap_value = 10,
            .element = EnemyType::Giant,
            .damage_range = {70, 100},
            .speed_range = {4, 7},
            .max_health = 500,
            .simple_hitbox_radius = 10.0f,
        };
    };

    template <typename T>
    concept IsEnemy = requires(T e, Enemy& enemy, const shapes::Circle& hitbox) {
        { T::info } -> std::same_as<const Info&>;
        // gets called if target hitbox collides or uncollides with enemy hitbox
        { e.tick(enemy, hitbox) } -> std::same_as<int>;
    };

}

struct Enemy {
    enum CollisionState {
        Collision,
        Uncollision,
        Unchanged,
    };

    Model model;
    ModelAnimation* anims;
    int anim_count;
    int anim_index = 0;
    int anim_curr_frame = 0;

    uint32_t health;
    uint32_t damage;
    uint16_t speed;

    Vector3 position;
    Vector2 movement;
    float angle;
    shapes::Circle simple_hitbox;
    CollisionState collision_state = Uncollision;

    // TODO: Stats are multiplied by some scaling factor and model is increased by 2x
    bool boss;

    template <enemies::IsEnemy... T> using State = std::variant<T...>;
    State<enemies::Paladin, enemies::Zombie, enemies::Heraklios, enemies::Maw> state;

    template <enemies::IsEnemy T>
    Enemy(Vector2 position, bool boss, T&& enemy)
        : model(LoadModel(T::info.model_path)), anims(LoadModelAnimations(T::info.model_path, &anim_count)),
          anim_index(T::info.default_anim), position((Vector3){position.x, T::info.y_component, position.y}),
          movement(Vector2Zero()),
          simple_hitbox(shapes::Circle(position, T::info.simple_hitbox_radius)), boss(boss), state(enemy) {
        model.transform = MatrixMultiply(model.transform, MatrixRotateX(std::numbers::pi / 2.0f));
        UpdateModelAnimation(model, anims[anim_index], anim_curr_frame);

        health = T::info.max_health;

        auto [min_speed, max_speed] = T::info.speed_range;
        speed = GetRandomValue(min_speed, max_speed);

        auto [min_damage, max_damage] = T::info.damage_range;
        damage = GetRandomValue(min_damage, max_damage);
    }

    void draw() const;
    void update_target(Vector2 new_target);
    // returned number is the amount of damage taken by the player
    int tick(shapes::Circle target_hitbox);

    void take_damage(uint32_t damage, Element element);
};

using Enemies = std::vector<Enemy>;
