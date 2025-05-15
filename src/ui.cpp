#include "ui.hpp"

#include "assets.hpp"
#include "font.hpp"
#include "hitbox.hpp"
#include "raylib.h"
#include "spell.hpp"
#include <cstdint>
#include <limits>

namespace ui {
    Button::Button(shapes::Polygon&& poly, std::function<void(State)> draw) : draw(draw), hitbox(std::move(poly)) {
    }

    bool Button::update(Mouse& mouse) {
        auto collision = check_collision(hitbox, mouse.mouse_pos);

        draw(collision ? Hover : Normal);

        return mouse.button_press && mouse.button_press->released_at &&
               check_collision(hitbox, mouse.button_press->pressed_at) &&
               check_collision(hitbox, *mouse.button_press->released_at) &&
               mouse.button_press->button == Mouse::Button::Left;
    }
}

namespace hud {
    int8_t SpellBar::dragged(Vector2 point, Vector3 dims, uint8_t unlocked_count) {
        if (point.x < dims.x || point.y < dims.y) return -1;

        for (uint8_t i = 1; i <= unlocked_count; i++) {
            if (point.x < (dims.x + dims.z * i)) return static_cast<int8_t>(i - 1);
        }

        return -1;
    }

    void SpellBar::draw(Vector3 dims, assets::Store& assets, const SpellBook& spellbook,
                        std::span<std::size_t> equipped) {
        DrawRectangle(static_cast<int>(dims.x), static_cast<int>(dims.y),
                      static_cast<int>(Player::max_spell_count * dims.z), static_cast<int>(dims.z), BLACK);

        for (std::size_t i = 0; i < equipped.size(); i++) {
            if (equipped[i] == std::numeric_limits<uint64_t>::max()) {
                assets.draw_texture(assets::EmptySpellSlot, Rectangle{
                                                                .x = dims.x + i * dims.z,
                                                                .y = dims.y,
                                                                .width = dims.z,
                                                                .height = dims.z,
                                                            });
                continue;
            }

            auto& spell = spellbook[equipped[i]];
            assets.draw_texture(spell.get_spell_tag(), Rectangle{
                                                           .x = dims.x + i * dims.z,
                                                           .y = dims.y,
                                                           .width = dims.z,
                                                           .height = dims.z,
                                                       });

            BeginBlendMode(BLEND_ADDITIVE);
            float cooldown_height = dims.z * static_cast<float>(spell.current_cooldown) / spell.cooldown;
            DrawRectangle(static_cast<int>(dims.x + static_cast<float>(i) * dims.z),
                          static_cast<int>(dims.y + dims.z - cooldown_height), static_cast<int>(dims.z),
                          static_cast<int>(cooldown_height), {130, 130, 130, 128});
            EndBlendMode();

            DrawTexturePro(assets[assets::SpellIconRarityFrame],
                           Rectangle{
                               .x = 0,
                               .y = 0,
                               .width = static_cast<float>(assets[assets::SpellIconRarityFrame].width),
                               .height = static_cast<float>(assets[assets::SpellIconRarityFrame].height),
                           },
                           Rectangle{
                               .x = dims.x + i * dims.z,
                               .y = dims.y,
                               .width = dims.z,
                               .height = dims.z,
                           },
                           Vector2Zero(), 0.0f, rarity::get_rarity_info(spell.rarity).color);
        }

        for (std::size_t i = equipped.size(); i < 10; i++) {
            assets.draw_texture(assets::LockedSlot, Rectangle{
                                                        .x = dims.x + i * dims.z,
                                                        .y = dims.y,
                                                        .width = dims.z,
                                                        .height = dims.z,
                                                    });
        }
    }

    void draw(assets::Store& assets, const Player& player, const SpellBook& spellbook, const Vector2& screen) {
        static const uint32_t padding = 10;
        static const uint32_t outer_radius = 1023 / 2;
        static const Vector2 center = (Vector2){outer_radius, outer_radius};

        BeginTextureMode(assets[assets::CircleUI]);
        ClearBackground(BLANK);

        DrawCircleV(center, outer_radius, BLACK);

        // EXP
        float angle = 360.0f * static_cast<float>(player.exp) / static_cast<float>(player.exp_to_next_lvl);
        DrawCircleV(center, outer_radius - 2 * padding, GRAY);
        DrawCircleSector(center, outer_radius - 2 * padding, -90.0f, angle - 90.0f, 512, Color{0, 255, 127, 255});

        // HEALTH & MANA
        // https://www.desmos.com/calculator/ltqvnvrd3p
        DrawCircleSector(center, outer_radius - 6 * padding, 90.0f, 270.0f, 512, Color{210, 4, 45, 255});
        DrawCircleSector(center, outer_radius - 6 * padding, -90.0f, 90.0f, 512, Color{93, 63, 211, 255});

        float health_s = static_cast<float>(player.health) / static_cast<float>(player.stats.max_health.get());
        float mana_s = static_cast<float>(player.mana) / static_cast<float>(player.stats.max_mana.get());
        float health_b = (1 + std::sin(std::numbers::pi_v<float> * health_s - std::numbers::pi_v<float> / 2.0f)) / 2.0f;
        float mana_b = (1 + std::sin(std::numbers::pi_v<float> * mana_s - std::numbers::pi_v<float> / 2.0f)) / 2.0f;
        int health_height = static_cast<int>(2.0f * (outer_radius - 6 * padding) * (1.0f - health_b));
        int mana_height = static_cast<int>(2.0f * (outer_radius - 6 * padding) * (1.0f - mana_b));

        // TODO: clean up this shit
        static const std::size_t segments = 512;
        for (std::size_t i = 0; i < segments; i++) {
            float health_x = (outer_radius - 6.0f * padding) -
                             (static_cast<float>(i) + 1.0f) * static_cast<float>(health_height) / segments;
            float mana_x = (outer_radius - 6.0f * padding) -
                           (static_cast<float>(i) + 1.0f) * static_cast<float>(mana_height) / segments;
            if (health_x < 0.0f)
                health_x = (outer_radius - 6.0f * padding) -
                           static_cast<float>(i) * static_cast<float>(health_height) / segments;
            if (mana_x < 0.0f)
                mana_x = (outer_radius - 6.0f * padding) -
                         static_cast<float>(i) * static_cast<float>(mana_height) / segments;
            int health_width = static_cast<int>(
                std::sqrt((outer_radius - 6.0f * padding) * (outer_radius - 6.0f * padding) - health_x * health_x));
            int mana_width = static_cast<int>(
                std::sqrt((outer_radius - 6.0f * padding) * (outer_radius - 6.0f * padding) - mana_x * mana_x));

            DrawRectangle(static_cast<int>(center.x) - health_width,
                          static_cast<int>(6.0f * padding +
                                           (static_cast<float>(health_height) / segments) * static_cast<float>(i)),
                          health_width, static_cast<int>(static_cast<float>(health_height) / segments + 1),
                          Color{169, 92, 104, 255});
            DrawRectangle(
                static_cast<int>(center.x),
                static_cast<int>(6.0f * padding + (static_cast<float>(mana_height) / segments) * static_cast<float>(i)),
                mana_width, static_cast<int>(static_cast<float>(mana_height) / segments + 1),
                Color{204, 204, 255, 255});
        }

        DrawRing(center, outer_radius - 6.2f * padding, outer_radius - 5 * padding, 0.0f, 360.0f, 512, BLACK);
        DrawLineEx((Vector2){center.x, 0.0f}, (Vector2){center.y, 1022.0f - 6 * padding}, 10.0f, BLACK);

        const auto inner_radius = outer_radius - 6 * padding;
        auto stat_dims = Vector2{
            .x = 0.0f,
            .y = 1023 * 0.3f,
        };
        auto stat_origin = Vector2{
            .x = center.x,
            .y = center.y - stat_dims.y / 2.0f,
        };
        stat_dims.x = std::sqrtf(stat_origin.y * (2 * inner_radius - stat_origin.y)) * 0.9f;
        stat_origin.x *= 1.04f;

        // MANA SIDE
        auto mana_current_text = std::to_string(player.mana);
        auto [mana_current_size, mana_current_dims] = font_manager::max_font_size(
            font_manager::Alagard, Vector2{stat_dims.x, stat_dims.y / 2.0f}, mana_current_text.data());
        auto mana_max_text = std::to_string(player.stats.max_mana.get());
        auto [mana_max_size, mana_max_dims] = font_manager::max_font_size(
            font_manager::Alagard, Vector2{stat_dims.x, stat_dims.y / 2.0f}, mana_max_text.data());

        auto bar_length = std::max(mana_current_dims.x, mana_max_dims.x) * 1.2f;
        DrawLineEx(
            Vector2{
                stat_origin.x + stat_dims.x / 2.0f - bar_length / 2.0f,
                center.y - 5.0f,
            },
            Vector2{
                stat_origin.x + stat_dims.x / 2.0f + bar_length / 2.0f,
                center.y - 5.0f,
            },
            10.0f, BLACK);
        font_manager::draw_text(mana_current_text.data(), font_manager::Alagard, mana_current_size,
                                static_cast<float>(mana_current_size) / 10.0f, WHITE,
                                Vector2{
                                    stat_origin.x + stat_dims.x / 2.0f - mana_current_dims.x / 2.0f,
                                    center.y - mana_current_dims.y,
                                },
                                font_manager::Exact);
        font_manager::draw_text(mana_max_text.data(), font_manager::Alagard, mana_max_size,
                                static_cast<float>(mana_max_size) / 10.0f, WHITE,
                                Vector2{
                                    stat_origin.x + stat_dims.x / 2.0f - mana_max_dims.x / 2.0f,
                                    center.y * 1.01f,
                                },
                                font_manager::Exact);

        // HEALTH SIDE
        stat_origin.x = center.x * 0.96f - stat_dims.x;
        auto health_current_text = std::to_string(player.health);
        auto [health_current_size, health_current_dims] = font_manager::max_font_size(
            font_manager::Alagard, Vector2{stat_dims.x, stat_dims.y / 2.0f}, health_current_text.data());
        auto health_max_text = std::to_string(player.stats.max_health.get());
        auto [health_max_size, health_max_dims] = font_manager::max_font_size(
            font_manager::Alagard, Vector2{stat_dims.x, stat_dims.y / 2.0f}, health_max_text.data());

        bar_length = std::max(health_current_dims.x, health_max_dims.x) * 1.2f;
        DrawLineEx(
            Vector2{
                stat_origin.x + stat_dims.x / 2.0f - bar_length / 2.0f,
                center.y - 5.0f,
            },
            Vector2{
                stat_origin.x + stat_dims.x / 2.0f + bar_length / 2.0f,
                center.y - 5.0f,
            },
            10.0f, BLACK);
        font_manager::draw_text(health_current_text.data(), font_manager::Alagard, health_current_size,
                                static_cast<float>(health_current_size) / 10.0f, WHITE,
                                Vector2{
                                    stat_origin.x + stat_dims.x / 2.0f - health_current_dims.x / 2.0f,
                                    center.y - health_current_dims.y,
                                },
                                font_manager::Exact);
        font_manager::draw_text(health_max_text.data(), font_manager::Alagard, health_max_size,
                                static_cast<float>(health_max_size) / 10.0f, WHITE,
                                Vector2{
                                    stat_origin.x + stat_dims.x / 2.0f - health_max_dims.x / 2.0f,
                                    center.y * 1.01f,
                                },
                                font_manager::Exact);

        EndTextureMode();
    }
}
