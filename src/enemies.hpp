#pragma once

#include "hitbox.hpp"
#include "quadtree.hpp"
#include "raylib.h"
#include "raymath.h"
#include "spell.hpp"
#include <cassert>
#include <cstdint>
#include <variant>

struct Enemy;

template <bool skip>
using QT = quadtree::QuadTree<5, Enemy, skip>;

namespace enemies {
    enum struct EnemyClass {
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
        EnemyClass element;
        std::pair<uint32_t, uint32_t> damage_range;
        std::pair<uint32_t, uint32_t> speed_range;
        uint32_t max_health;
        float simple_hitbox_radius;
        uint32_t base_exp_dropped;
    };

    struct Paladin {
        enum Animations {
            HeadButt = 0,
            Sprinting = 1,
        };

        int tick(QT<true>& enemies, std::size_t ix, const shapes::Circle target_hitbox);

        static constexpr Info info = (Info){
            .model_path = "./assets/paladin.glb",
            .model_scale = 0.3f,
            .default_anim = 1,
            .y_component = 1.0f,
            .cap_value = 2,
            .element = EnemyClass::Human,
            .damage_range = {15, 20},
            .speed_range = {2.5, 2.5},
            .max_health = 100,
            .simple_hitbox_radius = 10.0f,
            .base_exp_dropped = 20,
        };
    };

    struct Zombie {
        int tick(QT<true>& enemies, std::size_t ix, const shapes::Circle target_hitbox);

        static constexpr Info info = (Info){
            .model_path = "./assets/zombie.glb",
            .y_component = 1.0f,
            .cap_value = 1,
            .element = EnemyClass::Undead,
            .damage_range = {10, 20},
            .speed_range = {1, 3},
            .max_health = 50,
            .simple_hitbox_radius = 5.0f,
            .base_exp_dropped = 10,
        };
    };

    struct Heraklios {
        int tick(QT<true>& enemies, std::size_t ix, const shapes::Circle target_hitbox);

        static constexpr Info info = (Info){
            .model_path = "./assets/heraklios.glb",
            .model_scale = 0.2f,
            .default_anim = 0,
            .y_component = 1.0f,
            .cap_value = 5,
            .element = EnemyClass::Mage,
            .damage_range = {30, 50},
            .speed_range = {5, 10},
            .max_health = 300,
            .simple_hitbox_radius = 5.0f,
            .base_exp_dropped = 100,
        };
    };

    struct Maw {
        int tick(QT<true>& enemies, std::size_t ix, const shapes::Circle target_hitbox);

        static constexpr Info info = (Info){
            .model_path = "./assets/maw.glb",
            .model_scale = 0.2f,
            .default_anim = 0,
            .y_component = 1.0f,
            .cap_value = 10,
            .element = EnemyClass::Giant,
            .damage_range = {70, 100},
            .speed_range = {4, 7},
            .max_health = 500,
            .simple_hitbox_radius = 10.0f,
            .base_exp_dropped = 200,
        };
    };

    template <uint8_t N, typename T>
    concept IsEnemy = requires(T e, std::size_t ix, const shapes::Circle& hitbox, QT<true>& enemies) {
        { T::info } -> std::same_as<const Info&>;
        // gets called if target hitbox collides or uncollides with enemy hitbox
        { e.tick(enemies, ix, hitbox) } -> std::same_as<int>;
    };

#define EACH_ENEMY(F, G)                                                                                               \
    G(Paladin)                                                                                                         //\
    /*F(Zombie)                                                                                                          \*/
    /*F(Heraklios)                                                                                                       \*/
    /*F(Maw)*/

    // DON'T LOOK HERE, pwetty pwease OwO

#define ENEMY_ASSERT_CONCEPT(name) static_assert(IsEnemy<10, name>);
    EACH_ENEMY(ENEMY_ASSERT_CONCEPT, ENEMY_ASSERT_CONCEPT)
#undef ENEMY_ASSERT_CONCEPT

    enum class _EnemyType {
#define ENEMY_TYPE_FIRST(name) name = 0,
#define ENEMY_TYPE(name) name,
        EACH_ENEMY(ENEMY_TYPE, ENEMY_TYPE_FIRST) Size
#undef ENEMY_TYPE_FIRST
#undef ENEMY_TYPE
    };

    using State = std::variant<
#define ENEMY_VARIANT_FIRST(name) name
#define ENEMY_VARIANT(name) , name
        EACH_ENEMY(ENEMY_VARIANT, ENEMY_VARIANT_FIRST)
#undef ENEMY_VARIANT_FIRST
#undef ENEMY_VARIANT
        >;

    template <_EnemyType Type> struct EnemyFromEnum;

#define TAG_SPECIALIZE(name)                                                                                         \
    template <> struct EnemyFromEnum<_EnemyType::name> {                                                               \
        using Type = enemies::name;                                                                                    \
    };
    EACH_ENEMY(TAG_SPECIALIZE, TAG_SPECIALIZE)
#undef TAG_SPECIALIZE

    template <typename E> struct EnumFromEnemy;

#define ENEMY_SPECIALIZE(name) \
    template <> struct EnumFromEnemy<name> { \
        static constexpr _EnemyType type = _EnemyType::name; \
    };

    EACH_ENEMY(ENEMY_SPECIALIZE, ENEMY_SPECIALIZE)
#undef ENEMY_SPECIALIZE

    inline State create_enemy(_EnemyType type) {
        switch (type) {
#define ENEMY_CASE(name) case _EnemyType::name: return enemies::name();
            EACH_ENEMY(ENEMY_CASE, ENEMY_CASE)
#undef ENEMY_CASE
            case _EnemyType::Size:
                assert(false && "I hate you");
        }

        std::unreachable();
    }

    inline Info get_info(const _EnemyType& type) {
        switch (type) {
#define ENEMY_CASE(name) case _EnemyType::name: return enemies::name::info;
            EACH_ENEMY(ENEMY_CASE, ENEMY_CASE)
#undef ENEMY_CASE
            case _EnemyType::Size:
                assert(false && "I hate you");
        }

        std::unreachable();
    }

    static constexpr std::array<Info, static_cast<std::size_t>(_EnemyType::Size)> infos = {
#define ENEMY_INFO(name) name::info,
        EACH_ENEMY(ENEMY_INFO, ENEMY_INFO)
#undef ENEMY_INFO
    };

#undef EACH_ENEMY

    // Under here it's safe
    std::optional<State> random_enemy(uint32_t available_cap, uint32_t& cap);
    Info get_info(const State& state);
    _EnemyType get_type(const State& state);
}

class EnemyModels {
  public:
    struct Animation {
        ModelAnimation* animations;
        int count;
    };

    EnemyModels();
    EnemyModels(EnemyModels&) = delete;

    std::pair<Model, Animation> operator[](const enemies::State& state);

    ~EnemyModels();

  private:
    std::array<std::pair<Model, Animation>, static_cast<int>(enemies::_EnemyType::Size)> models;
};

struct Enemy {
    enum CollisionState {
        Collision,
        Uncollision,
        Unchanged,
    };

    int anim_index = 0;
    int anim_curr_frame = 0;

    uint32_t health;
    uint32_t damage;
    uint16_t speed;
    uint16_t level;

    Vector3 pos;
    Vector2 movement;
    float angle;
    shapes::Circle simple_hitbox;
    CollisionState collision_state = Uncollision;

    // TODO: Stats are multiplied by some scaling factor and model is increased by 2x
    bool boss;

    enemies::State state;

    Enemy(Vector2 position, uint16_t level, bool boss, enemies::State&& enemy)
        : anim_index(get_info(enemy).default_anim), level(level),
          pos((Vector3){position.x, get_info(enemy).y_component, position.y}), movement(Vector2Zero()),
          simple_hitbox(shapes::Circle(position, get_info(enemy).simple_hitbox_radius)), boss(boss), state(enemy) {
        auto info = get_info(enemy);
        health = info.max_health;

        // TODO: scale based on `level`
        auto [min_speed, max_speed] = info.speed_range;
        speed = GetRandomValue(min_speed, max_speed);

        auto [min_damage, max_damage] = info.damage_range;
        damage = GetRandomValue(min_damage, max_damage);
    }

    Enemy(const Enemy&) = default;
    Enemy(Enemy&& enemy) noexcept
        : anim_index(enemy.anim_index),
          anim_curr_frame(enemy.anim_curr_frame), health(enemy.health), damage(enemy.damage), speed(enemy.speed),
          pos(enemy.pos), movement(enemy.movement), angle(enemy.angle),
          simple_hitbox(std::move(enemy.simple_hitbox)), collision_state(enemy.collision_state), boss(enemy.boss),
          state(std::move(enemy.state)) {
    };

    Enemy& operator=(const Enemy&) = default;
    Enemy& operator=(Enemy&&) = default;

    void draw(EnemyModels& enemy_models, const Vector3& offset) const;
    void update_target(QT<true>& enemies, Vector2 player_pos, std::size_t ix);
    // returned number is the amount of damage taken by the player
    uint32_t tick(QT<true>& enemies, std::size_t ix, shapes::Circle target_hitbox, EnemyModels& enemy_models);

    // if not nullopt, then the enemy is dead and dropped uint32_t amount of exp
    std::optional<uint32_t> take_damage(uint32_t damage, Element element);

    Vector2 position() const;
    void set_position(const Vector2& p);
    uint32_t dropped_exp() const;
};
