#include "enemies.hpp"
#include "hitbox.hpp"
#include "rayhacks.hpp"
#include "raylib.h"
#include "raymath.h"
#include "utility.hpp"
#include <cassert>

namespace enemies {
    int Paladin::tick(Enemy& data, const shapes::Polygon target_hitbox) {
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

    int Zombie::tick(Enemy& data, const shapes::Polygon target_hitbox) {
        return 0;
    }

    int Heraklios::tick(Enemy& data, const shapes::Polygon target_hitbox) {
        return 0;
    }

    int Maw::tick(Enemy& data, const shapes::Polygon target_hitbox) {
        return 0;
    }

    void Enemy::draw() const {
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
        angle = angle_from_point(new_target, (Vector2){ position.x, position.z });
    }

    int Enemy::tick(shapes::Polygon target_hitbox) {
        return std::visit(
            [&](auto&& arg) {
                position.x += movement.x * speed;
                position.z += movement.y * speed;

                simple_hitbox.center.x = position.x;
                simple_hitbox.center.y = position.z;

                anim_curr_frame = (anim_curr_frame + 3) % anims[anim_index].frameCount;
                UpdateModelAnimation(model, anims[anim_index], anim_curr_frame);

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
}
