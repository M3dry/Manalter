#include "ui.hpp"

#include "assets.hpp"
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
    SpellBookUI::SpellBookUI(Vector2 tile_dims, const SpellBook& spellbook, const Vector2& screen) {
        area.width = screen.x * 0.27f;
        area.height = screen.y * 0.9f;
        area.x = 0;
        area.y = 0;

        spell_dims.x = area.width * 0.7f;
        spell_dims.y = spell_dims.x * tile_dims.y / tile_dims.x;
        page_size = static_cast<std::size_t>((area.height * 0.9f) / spell_dims.y);
        spells = {0, std::min(page_size, spellbook.size())};

        hitboxes.reserve(page_size);
        for (std::size_t i = 0; i < page_size; i++) {
            hitboxes.emplace_back(ui::Draggable<assets::Store&, const Spell&, const SpellBookUI&>(
                {area.x, area.y + i * spell_dims.y},
                (Rectangle){area.x, area.y + i * spell_dims.y, area.width, spell_dims.y},
                [](auto origin, auto& assets, const Spell& spell, const auto& ui) {
                    return ui.draw_spell(assets, origin, spell);
                }));
        }
    }

    std::optional<std::pair<uint64_t, Vector2>> SpellBookUI::update([[maybe_unused]] assets::Store& assets,
                                                                    const SpellBook& spellbook, Mouse& mouse,
                                                                    [[maybe_unused]] std::optional<Vector2> screen) {
        /*DrawRectangleRec(area, WHITE);*/
        assets.draw_texture(assets::SpellBookBackground, area);

        auto& [first, second] = spells;
        auto spellbook_size = spellbook.size();
        if (mouse.wheel_movement != 0) {
            if (mouse.wheel_movement < 0) {
                first += static_cast<std::size_t>(-mouse.wheel_movement);
                second += static_cast<std::size_t>(-mouse.wheel_movement);

                if (second > spellbook_size) {
                    first = spellbook_size - page_size;
                    second = spellbook_size;
                }
            } else if (first >= static_cast<std::size_t>(mouse.wheel_movement)) {
                first -= static_cast<std::size_t>(mouse.wheel_movement);
                second -= static_cast<std::size_t>(mouse.wheel_movement);
            } else {
                first = 0;
                second = std::min(page_size, spellbook_size);
            }
        }

        std::optional<std::pair<uint64_t, Vector2>> ret;
        for (std::size_t spell_id = first; spell_id < second; spell_id++) {
            if (auto vec = hitboxes[spell_id - first].update(mouse, assets, spellbook[spell_id], *this); vec) {
                assert(!ret && "the impossible happened"); // shouldn't be possible to drag two things at once
                ret = {spell_id, *vec};
            }
        }

        return ret;
    }

    void SpellBookUI::draw_spell(assets::Store& assets, Vector2 origin, const Spell& spell) const {
        auto working_area = Rectangle{
            .x = origin.x,
            .y = origin.y,
            .width = spell_dims.x,
            .height = spell_dims.y,
        };
        DrawRectangleRec(working_area, WHITE);

        working_area.x += 3;
        working_area.y += 3;
        working_area.width -= 6;
        working_area.height -= 6;
        assets.draw_texture(assets::SpellTileBackground, working_area);

        auto spell_info = spell.get_spell_info();
        auto spell_icon = assets[spell.get_spell_tag()];
        assets.draw_texture(spell.get_spell_tag(), Rectangle{
               .x = working_area.x,
               .y = working_area.y,
               .width = working_area.height * 0.7f,
               .height = working_area.height * 0.7f,
        });
        DrawTexturePro(assets[assets::SpellTileRarityFrame],
                       Rectangle{
                           .x = 0.0f,
                           .y = 0.0f,
                           .width = static_cast<float>(assets[assets::SpellTileRarityFrame].width),
                           .height = static_cast<float>(assets[assets::SpellTileRarityFrame].height),
                       },
                       working_area, Vector2Zero(), 0.0f, rarity::get_rarity_info(spell.rarity).color);
    }

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

        BeginTextureMode(assets[assets::CircleUI, false]);
        ClearBackground(BLANK);

        DrawCircleV(center, outer_radius, BLACK);

        // EXP
        float angle = 360.0f * static_cast<float>(player.exp) / static_cast<float>(player.exp_to_next_lvl);
        DrawCircleV(center, outer_radius - 2 * padding, WHITE);
        DrawCircleSector(center, outer_radius - 2 * padding, -90.0f, angle - 90.0f, 512, GREEN);

        // HEALTH & MANA
        // https://www.desmos.com/calculator/ltqvnvrd3p
        DrawCircleSector(center, outer_radius - 6 * padding, 90.0f, 270.0f, 512, RED);
        DrawCircleSector(center, outer_radius - 6 * padding, -90.0f, 90.0f, 512, BLUE);

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
                          health_width, static_cast<int>(static_cast<float>(health_height) / segments + 1), WHITE);
            DrawRectangle(
                static_cast<int>(center.x),
                static_cast<int>(6.0f * padding + (static_cast<float>(mana_height) / segments) * static_cast<float>(i)),
                mana_width, static_cast<int>(static_cast<float>(mana_height) / segments + 1), WHITE);
        }

        DrawRing(center, outer_radius - 6.2f * padding, outer_radius - 5 * padding, 0.0f, 360.0f, 512, BLACK);
        DrawLineEx((Vector2){center.x, 0.0f}, (Vector2){center.y, 1022.0f - 6 * padding}, 10.0f, BLACK);
        EndTextureMode();
        EndTextureModeMSAA(assets[assets::CircleUI, false], assets[assets::CircleUI, true]);
    }
}
