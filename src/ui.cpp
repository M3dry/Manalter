#include "ui.hpp"

#include "assets.hpp"
#include "hitbox.hpp"
#include "raylib.h"
#include "spell.hpp"
#include "utility.hpp"
#include <cstdint>
#include <format>
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
    SpellBookUI::SpellBookUI(const SpellBook& spellbook, const Vector2& screen)
        : spells(0, std::min(page_size, spellbook.size())) {
        area.width = screen.x * 0.27f;
        area.height = screen.y * 0.9f;
        area.x = screen.x * 0.01f;
        area.y = (screen.y - area.height) * 0.5f;

        spell_dims.x = area.width;
        spell_dims.y = area.height / page_size;

        hitboxes.reserve(page_size);
        for (std::size_t i = 0; i < page_size; i++) {
            hitboxes.emplace_back(ui::Draggable<const SpellBook&, const SpellBookUI&>(
                {area.x, area.y + i * spell_dims.y},
                (Rectangle){area.x, area.y + i * spell_dims.y, area.width, spell_dims.y},
                [i](auto origin, const SpellBook& spellbook, const auto& ui) {
                    return ui.draw_spell(origin, i, spellbook);
                }));
        }
    }

    std::optional<std::pair<uint64_t, Vector2>> SpellBookUI::update([[maybe_unused]] assets::Store& assets,
                                                                    const SpellBook& spellbook, Mouse& mouse,
                                                                    [[maybe_unused]] std::optional<Vector2> screen) {
        DrawRectangleRec(area, WHITE);

        auto& [first, second] = spells;
        auto spellbook_size = spellbook.size();
        /*if (mouse.wheel_movement != 0) {*/
        /*    if (mouse.wheel_movement < 0) {*/
        /*        if (first <= mouse.wheel_movement)*/
        /*            first = 0;*/
        /*        else*/
        /*            first += mouse.wheel_movement;*/
        /**/
        /*        if (second <= mouse.wheel_movement)*/
        /*            second = 0;*/
        /*        else*/
        /*            second += mouse.wheel_movement;*/
        /*    } else {*/
        /*        first += mouse.wheel_movement;*/
        /*        second += mouse.wheel_movement;*/
        /*    }*/
        /**/
        /*    if (first == 0) {*/
        /*        second = std::min(page_size, spellbook_size);*/
        /*    } else if (second > spellbook_size) {*/
        /*        first = spellbook_size <= page_size ? 0 : spellbook.size() - page_size;*/
        /*        second = spellbook_size;*/
        /*    }*/
        /*}*/

        std::optional<std::pair<uint64_t, Vector2>> ret;
        for (std::size_t spell_id = first; spell_id < second; spell_id++) {
            if (auto vec = hitboxes[spell_id - first].update(mouse, spellbook, *this); vec) {
                assert(!ret && "the impossible happened"); // shouldn't be possible to drag two things at once
                ret = {spell_id, *vec};
            }
        }
        DrawText(std::format("[{}, {}); MAX: {}", first, second, spellbook.size()).c_str(), static_cast<int>(area.x),
                 static_cast<int>(area.y), 20, RED);

        return ret;
    }

    void SpellBookUI::draw_spell(Vector2 origin, uint64_t id, [[maybe_unused]] const SpellBook& spellbook) const {
        DrawRectangle(static_cast<int>(origin.x), static_cast<int>(origin.y), static_cast<int>(spell_dims.x),
                      static_cast<int>(spell_dims.y), BLACK);
        DrawText(std::format("ORDER: {}", id).c_str(), static_cast<int>(origin.x + spell_dims.x / 2.0f),
                 static_cast<int>(origin.y + spell_dims.y / 2.0f), 10, BLACK);
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

            assets.draw_texture(spell.rarity, Rectangle{
                                                  .x = dims.x + i * dims.z,
                                                  .y = dims.y,
                                                  .width = dims.z,
                                                  .height = dims.z,
                                              });
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

        // Spell Bar
        BeginTextureMode(assets[assets::SpellBarUI, false]);
        ClearBackground(BLANK);

        DrawRectangle(0, 0, 512, 128, RED);

        const int spell_dim = 128 / 2; // 2x5 spells

        for (int i = 0; i < 10; i++) {
            int row = i / 5;
            int col = i - 5 * row;

            if (i >= player.unlocked_spell_count) {
                assets.draw_texture(assets::LockedSlot, (Rectangle){(float)col * spell_dim, (float)spell_dim * row,
                                                                    (float)spell_dim, (float)spell_dim});
                continue;
            }

            if (auto spell_opt = player.get_equipped_spell(i, spellbook); spell_opt) {
                const Spell& spell = *spell_opt;

                // TODO: rn ignoring the fact that the frame draws over the spell
                // icon
                assets.draw_texture(spell.get_spell_tag(), (Rectangle){(float)col * spell_dim, (float)spell_dim * row,
                                                                       (float)spell_dim, (float)spell_dim});
                BeginBlendMode(BLEND_ADDITIVE);
                float cooldown_height = spell_dim * (float)spell.current_cooldown / spell.cooldown;
                DrawRectangle(col * spell_dim, spell_dim * row + (spell_dim - cooldown_height), spell_dim,
                              cooldown_height, {130, 130, 130, 128});
                EndBlendMode();

                assets.draw_texture(spell.rarity, (Rectangle){(float)col * spell_dim, (float)spell_dim * row,
                                                              (float)spell_dim, (float)spell_dim});
            } else {
                assets.draw_texture(assets::EmptySpellSlot, (Rectangle){(float)col * spell_dim, (float)row * spell_dim,
                                                                        (float)spell_dim, (float)spell_dim});
            }
        }

        // RADAR/MAP???
        DrawRectangle(spell_dim * 5, 0, 512 - spell_dim * 5, 128, GRAY);
        EndTextureMode();
        EndTextureModeMSAA(assets[assets::SpellBarUI, false], assets[assets::SpellBarUI, true]);

        Vector2 spellbook_dims =
            (Vector2){static_cast<float>(SpellBookWidth(screen)), static_cast<float>(SpellBookHeight(screen))};
        BeginTextureMode(assets[assets::SpellBookUI, true]);
        DrawRectangle(0, 0, spellbook_dims.x, spellbook_dims.y, BLACK);
        spellbook_dims -= 2.0f;
        DrawRectangle(1, 1, spellbook_dims.x, spellbook_dims.y, WHITE);

        spellbook_dims -= 6.0f;
        int spell_height = spellbook_dims.x / 5.0f;
        Rectangle spell_dims;
        std::size_t total_spells = spellbook.size();
        std::size_t page_size =
            std::min(static_cast<std::size_t>(spellbook_dims.y / static_cast<float>(spell_height) - 1.0f),
                     static_cast<std::size_t>(total_spells));
        for (std::size_t i = 0; i < page_size; i++) {
            auto rarity_color = rarity::get_rarity_info(spellbook[i].rarity).color;

            // outer border
            spell_dims = (Rectangle){4.0f, 4.0f + i * 2.0f + i * spell_height, spellbook_dims.x, (float)spell_height};
            DrawRectangleRec(spell_dims, rarity_color);

            // usable space for spell info
            spell_dims.x += 3.0f;
            spell_dims.y += 3.0f;
            spell_dims.width -= 6.0f;
            spell_dims.height -= 6.0f;
            DrawRectangleRec(spell_dims, WHITE);

            // spell icon
            assets.draw_texture(spellbook[i].get_spell_tag(),
                                (Rectangle){spell_dims.x, spell_dims.y, spell_dims.height, spell_dims.height});
            spell_dims.x += spell_dims.height;
            spell_dims.width -= spell_dims.height + 3.0f;
            DrawRectangle(static_cast<int>(spell_dims.x), static_cast<int>(spell_dims.y), 3.0f,
                          static_cast<int>(spell_dims.height), rarity_color);
            spell_dims.x += 3.0f;

            // name bar
            Rectangle name_dims = spell_dims;
            name_dims.height = std::floor(spell_dims.height / 3);

            // stat bar
            Rectangle stat_bar = spell_dims;
            stat_bar.height = spell_dims.height - name_dims.height;
            stat_bar.y += name_dims.height;

            // element icon
            Rectangle icon_dims = stat_bar;
            icon_dims.width = icon_dims.height;
            stat_bar.x += icon_dims.width;
            stat_bar.width -= icon_dims.width;

            // exp bar
            Rectangle exp_dims = stat_bar;
            exp_dims.height = std::floor(stat_bar.height / 3);
            stat_bar.height -= exp_dims.height;
            stat_bar.y += exp_dims.height;

            DrawRectangleRec(icon_dims, YELLOW);
            DrawRectangleRec(exp_dims, GREEN);
            DrawRectangleRec(stat_bar, RED);

            auto spell_name = spellbook[i].get_spell_info().name;
            auto [name_font_size, name_text_dims] =
                max_font_size(assets[assets::Macondo], 1.0f, (Vector2){name_dims.width, name_dims.height}, spell_name);
            name_text_dims.x = name_dims.x + name_dims.width / 2.0f - name_text_dims.x / 2.0f;
            name_text_dims.y = name_dims.y;
            DrawTextEx(assets[assets::Macondo], spell_name, name_text_dims, static_cast<float>(name_font_size), 1.0f,
                       rarity_color);
        }
        EndTextureMode();
        // EndTextureModeMSAA(assets[assets::SpellBookUI, false],
        // assets[assets::SpellBookUI, true]);
    }
}
