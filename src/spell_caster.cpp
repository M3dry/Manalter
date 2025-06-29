#include "spell_caster.hpp"
#include "particle/effects.hpp"
#include "hitbox.hpp"
#include "spell.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <raylib.h>
#include <raymath.h>
#include <vector>

namespace caster {
    struct Moving {
        Moving(std::size_t spell_id, const spell::movement::Beam& movement_info, const Vector2& movement,
               const Vector2& origin, const Vector2& mouse_position)
            : segment_length(movement_info.speed), spell_id(spell_id), till_removal(movement_info.duration),
              till_stopped(movement_info.stop_after), create_segments(movement_info.length / segment_length),
              movement(movement), hitbox([&]() -> std::vector<Vector2> {
                  // FIX: Maybe do something nicer, but really screw oop

                  // time for some serious math
                  // https://desmos.com/calculator/81840tu4jg
                  auto width = movement_info.width;
                  Vector2 relative = (Vector2){mouse_position.x - origin.x, origin.y - mouse_position.y};
                  float angle = std::atan(relative.y / relative.x);
                  auto c = [width](float alpha) {
                      return (Vector2){width / 2.0f * std::sin(alpha), width / 2.0f * std::cos(alpha)};
                  };

                  Vector2 a = c(-angle), b = c(static_cast<float>(std::numbers::pi) - angle);
                  a = (Vector2){origin.x + a.x, origin.y - a.y};
                  b = (Vector2){origin.x + b.x, origin.y - b.y};

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
        bool tick(const SpellBook& spellbook, Enemies& enemies, std::vector<ItemDrop>& item_drops) {
            if (till_stopped > 0) {
                if (create_segments > 0) {
                    hitbox.points[0] += movement;
                    hitbox.points[1] += movement;
                    create_segments--;
                } else {
                    hitbox.translate(movement);
                }
                till_stopped--;
            }

            auto spell_exp = enemies.deal_damage(hitbox, spellbook[spell_id].stats.damage.get(),
                                                 spellbook[spell_id].get_spell_info().element, item_drops);
            spellbook[spell_id].add_exp(spell_exp);

            if (till_stopped <= 0)
                return till_removal-- == 0;
            else
                return false;
        }

        uint16_t segment_length;
        std::size_t spell_id;
        // in ticks, when 0 remove spell
        uint16_t till_removal;
        // in units, when 0 stop moving the spell
        uint16_t till_stopped;
        // when 0 stop creating segments
        uint16_t create_segments;

        Vector2 movement;
        // 0, 1 points are the head
        // 2, 3 points are the tail
        shapes::Polygon hitbox;
    };

    struct Circle {
        Circle(std::size_t spell_id, Vector2 center, spell::movement::Circle info)
            : spell_id(spell_id), hitbox(center, info.initial_radius), until_max(info.increase_duration),
              radius_increase(static_cast<float>(info.maximal_radius - info.initial_radius) / info.increase_duration),
              wait(info.duration) {
        }

        bool tick(const SpellBook& spellbook, Enemies& enemies, std::vector<ItemDrop>& item_drops) {
            if (until_max > 0) {
                hitbox.radius += radius_increase;
                until_max--;
            }

            auto spell_exp = enemies.deal_damage(hitbox, spellbook[spell_id].stats.damage.get(),
                                spellbook[spell_id].get_spell_info().element, item_drops);
            spellbook[spell_id].add_exp(spell_exp);

            if (until_max == 0) return wait-- == 0;
            return false;
        }

        std::size_t spell_id;
        shapes::Circle hitbox;
        uint8_t until_max;
        float radius_increase;
        uint16_t wait;
    };

    std::vector<Moving> moving_spells = {};
    std::vector<Circle> circle_spells = {};

    std::optional<Vector2> point(spell::movement::Point point, Vector2 mouse, Vector2 player, const Enemies& enemies) {
        switch (point) {
            case spell::movement::Mouse:
                return mouse;
            case spell::movement::Player:
                return player;
            case spell::movement::ClosestEnemy:
                if (auto enemy_ix = enemies.enemies.closest_to(player); enemy_ix)
                    return xz_component(enemies.enemies.data.vec[*enemy_ix].val.pos);
                return std::nullopt;
        }

        std::unreachable();
    }

    void clear() {
        moving_spells.clear();
        moving_spells.shrink_to_fit();

        circle_spells.clear();
        circle_spells.shrink_to_fit();
    }

    bool cast(std::size_t spell_id, const Spell& spell, const Vector2& player_position, const Vector2& mouse_position,
              const Enemies& enemies) {
        auto info = spell.get_spell_info();
        return std::visit(
            [&](auto&& arg) -> bool {
                using T = std::decay_t<decltype(arg)>;

                Vector2 effect_origin{};
                if constexpr (std::is_same_v<T, spell::movement::Circle>) {
                    auto center = point(arg.center, mouse_position, player_position, enemies);
                    if (!center) return false;

                    circle_spells.emplace_back(spell_id, *center, arg);
                    effect_origin = *center;
                } else if constexpr (std::is_same_v<T, spell::movement::Beam>) {
                    auto origin = point(arg.origin, mouse_position, player_position, enemies);
                    auto dest = point(arg.dest, mouse_position, player_position, enemies);
                    if (!origin || !dest) return false;

                    moving_spells.emplace_back(spell_id, arg, Vector2Normalize(*dest - *origin), player_position,
                                               mouse_position);
                    effect_origin = *origin;
                }

                std::visit([&](auto&& eff) { effects::push_effect(eff(effect_origin)); }, info.effect);
                return true;
            },
            info.movement);
    }

    void tick(const SpellBook& spellbook, Enemies& enemies, std::vector<ItemDrop>& item_drops) {
        {
            auto [first, last] =
                std::ranges::remove_if(circle_spells, [&spellbook, &enemies, &item_drops](auto& circle) {
                    return circle.tick(spellbook, enemies, item_drops);
                });
            circle_spells.erase(first, last);
        }

        {
            auto [first, last] =
                std::ranges::remove_if(moving_spells, [&spellbook, &enemies, &item_drops](auto& moving) {
                    return moving.tick(spellbook, enemies, item_drops);
                });
            moving_spells.erase(first, last);
        }
    }

#ifdef DEBUG
    void draw_hitbox(float y) {
        for (const auto& circle : circle_spells) {
            circle.hitbox.draw_3D(RED, y, Vector2Zero());
        }

        for (const auto& moving : moving_spells) {
            moving.hitbox.draw_lines_3D(RED, y);
        }
    }
#endif
}
