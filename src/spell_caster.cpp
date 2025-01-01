#include "spell_caster.hpp"
#include "hitbox.hpp"
#include "rayhacks.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <print>
#include <raylib.h>
#include <raymath.h>
#include <tuple>
#include <vector>

namespace caster {
    struct Moving {
        Moving(std::size_t spell_id, const Spell::SpellToMouse& movement_info, const Vector2& movement,
               const Vector2& origin, const Vector2& mouse_position)
            : segment_length(movement_info.speed), spell_id(spell_id), till_removal(movement_info.duration),
              till_stopped(movement_info.stop_after), create_segments(movement_info.length / segment_length),
              origin(origin), movement(movement), hitbox([&]() -> std::vector<Vector2> {
                  // FIX: Maybe do something nicer, but really screw oop

                  // time for some serious math
                  // https://desmos.com/calculator/81840tu4jg
                  auto width = movement_info.width;
                  Vector2 relative = (Vector2){mouse_position.x - origin.x, origin.y - mouse_position.y};
                  float angle = std::atan(relative.y / relative.x);
                  auto c = [width](float alpha) {
                      return (Vector2){width / 2.0f * std::sin(alpha), width / 2.0f * std::cos(alpha)};
                  };

                  Vector2 a = c(-angle), b = c(std::numbers::pi - angle);
                  a = (Vector2){origin.x + a.x, origin.y - a.y};
                  b = (Vector2){origin.x + b.x, origin.y - b.y};

                  std::println("ANGLE: {}", angle * 180.0f / std::numbers::pi);
                  std::println("MOVEMENT: [{};{}]", movement.x, movement.y);
                  std::println("ORIGIN: [{};{}]", origin.x, origin.y);
                  std::println("RELATIVE: [{};{}]", relative.x, relative.y);
                  std::println("A: [{};{}]", a.x, a.y);
                  std::println("A: [{};{}]", b.x, b.y);

                  return {
                      // head
                      a,
                      b,
                      // tail
                      b,
                      a,
                  };
              }()) {
            this->movement *= segment_length;
        }

        // when true spell finished
        bool tick() {
            if (till_stopped > 0) {
                if (create_segments > 0) {
                    hitbox.points[0] += movement;
                    hitbox.points[1] += movement;
                    create_segments--;
                } else {
                    hitbox.update(movement);
                }
                till_stopped--;
            }

            return till_removal-- == 0;
        }

        uint16_t segment_length;
        std::size_t spell_id;
        // in ticks, when 0 remove spell
        uint16_t till_removal;
        // in units, when 0 stop moving the spell
        uint16_t till_stopped;
        // when 0 stop creating segments
        uint16_t create_segments;

        Vector2 origin;
        Vector2 movement;
        // 0, 1 points are the head
        // 2, 3 points are the tail
        shapes::Polygon hitbox;
    };

    std::vector<Moving> moving_spells = {};
    std::vector<std::tuple<std::size_t, shapes::Circle>> circle_spells = {};

    void cast(std::size_t spell_id, const Spell& spell, const Vector2& player_position, const Vector2& mouse_position) {
        auto reach = spell.get_reach();

        switch (spell.get_type()) {
            case Spell::AtMouse: {
                float radius = std::get<0>(reach);
                circle_spells.emplace_back(spell_id, shapes::Circle(mouse_position, radius));
                break;
            }
            case Spell::AtPlayer: {
                float radius = std::get<0>(reach);
                circle_spells.emplace_back(spell_id, shapes::Circle(player_position, radius));
                break;
            }
            case Spell::ToMouse: {
                Vector2 movement = Vector2Normalize(mouse_position - player_position);
                moving_spells.emplace_back(spell_id, std::get<1>(reach), movement, player_position, mouse_position);
                break;
            }
        }
    }

    void tick(const SpellBook& spellbook) {
        for (auto [spell_id, circle] : circle_spells) {
            // TODO: check for collisions between enemies and the `circle` shape
        }
        circle_spells.clear();

        auto [first, last] = std::ranges::remove_if(moving_spells, [](auto& moving) { return moving.tick(/* NOTE: pass enemies here, IG? */); });
        moving_spells.erase(first, last);
    }

#ifdef DEBUG
    void draw_hitbox(float y) {
        for (auto [_, circle] : circle_spells) {
            circle.draw_3D(RED, y);
        }

        for (const auto& moving : moving_spells) {
            moving.hitbox.draw_lines_3D(RED, y);
        }
    }
#endif
}
