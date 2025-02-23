#include "ui.hpp"

#include "raylib.h"

#include "hitbox.hpp"
#include "rayhacks.hpp"
#include "utility.hpp"

namespace ui {
    Button::Button(shapes::Polygon&& poly, std::function<void(State)> draw) : draw(draw), hitbox(std::move(poly)) {
    }

    bool Button::update(Mouse& mouse) {
        auto collision = check_collision(hitbox, mouse.mouse_pos);

        draw(collision ? Hover : Normal);

        return mouse.button_press && mouse.button_press->released_at &&
                   check_collision(hitbox, mouse.button_press->pressed_at),
               check_collision(hitbox, *mouse.button_press->released_at) &&
                   mouse.button_press->button == Mouse::Button::Left;
    }

    Draggable::Draggable(Vector2 origin, shapes::Polygon&& poly, std::function<void(Vector2)> draw)
        : draw(draw), origin(origin), hitbox(std::move(poly)) {
    }

    std::optional<Vector2> Draggable::update(Mouse& mouse) {
        if (mouse.button_press && mouse.button_press->button == Mouse::Button::Left) {
            if (mouse.button_press->released_at) return *mouse.button_press->released_at;

            draw(origin + (mouse.mouse_pos - mouse.button_press->pressed_at));
        } else {
            draw(origin);
        }

        return std::nullopt;
    }
}

namespace hud {
    void SpellBookUI::draw(assets::Store& assets, const SpellBook& spellbook) {

    }

    void SpellBookUI::update(Mouse& mouse, std::optional<Vector2> screen_update) {

    }

    void draw(assets::Store& assets, const Player& player, const SpellBook& spellbook, const Vector2& screen) {
        static const uint32_t padding = 10;
        static const uint32_t outer_radius = 1023 / 2;
        static const Vector2 center = (Vector2){(float)outer_radius, (float)outer_radius};

        BeginTextureMode(assets[assets::CircleUI, false]);
        ClearBackground(BLANK);

        DrawCircleV(center, outer_radius, BLACK);

        // EXP
        float angle = 360.0f * (float)player.exp / player.exp_to_next_lvl;
        DrawCircleV(center, outer_radius - 2 * padding, WHITE);
        DrawCircleSector(center, outer_radius - 2 * padding, -90.0f, angle - 90.0f, 512, GREEN);

        // HEALTH & MANA
        // https://www.desmos.com/calculator/ltqvnvrd3p
        DrawCircleSector(center, outer_radius - 6 * padding, 90.0f, 270.0f, 512, RED);
        DrawCircleSector(center, outer_radius - 6 * padding, -90.0f, 90.0f, 512, BLUE);

        float health_s = (float)player.health / (float)player.max_health;
        float mana_s = (float)player.mana / (float)player.max_mana;
        float health_b =
            (1 + std::sin(std::numbers::pi_v<double> * health_s - std::numbers::pi_v<double> / 2.0f)) / 2.0f;
        float mana_b = (1 + std::sin(std::numbers::pi_v<double> * mana_s - std::numbers::pi_v<double> / 2.0f)) / 2.0f;
        int health_height = 2 * (outer_radius - 6 * padding) * (1 - health_b);
        int mana_height = 2 * (outer_radius - 6 * padding) * (1 - mana_b);

        // TODO: clean up this shit
        static const float segments = 512.0f;
        for (int i = 0; i < segments; i++) {
            float health_x = (outer_radius - 6.0f * padding) - (i + 1.0f) * health_height / segments;
            float mana_x = (outer_radius - 6.0f * padding) - (i + 1.0f) * mana_height / segments;
            if (health_x < 0.0f) health_x = (outer_radius - 6.0f * padding) - i * health_height / segments;
            if (mana_x < 0.0f) mana_x = (outer_radius - 6.0f * padding) - i * mana_height / segments;
            float health_width =
                std::sqrt((outer_radius - 6.0f * padding) * (outer_radius - 6.0f * padding) - health_x * health_x);
            float mana_width =
                std::sqrt((outer_radius - 6.0f * padding) * (outer_radius - 6.0f * padding) - mana_x * mana_x);

            DrawRectangle(center.x - health_width, 6.0f * padding + (health_height / segments) * i, health_width,
                          health_height / segments + 1, WHITE);
            DrawRectangle(center.x, 6.0f * padding + (mana_height / segments) * i, mana_width,
                          mana_height / segments + 1, WHITE);
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

            if (i >= player.max_spells) {
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

        Vector2 spellbook_dims = (Vector2){SpellBookWidth(screen), SpellBookHeight(screen)};
        BeginTextureMode(assets[assets::SpellBookUI, true]);
        DrawRectangle(0, 0, spellbook_dims.x, spellbook_dims.y, BLACK);
        spellbook_dims -= 2.0f;
        DrawRectangle(1, 1, spellbook_dims.x, spellbook_dims.y, WHITE);

        spellbook_dims -= 6.0f;
        int spell_height = spellbook_dims.x / 5.0f;
        int i;
        Rectangle spell_dims;
        int total_spells = spellbook.size();
        int page_size = std::min((int)(spellbook_dims.y / spell_height - 1), total_spells);
        for (i = 0; i < page_size; i++) {
            auto rarity_color = spellbook[i].get_rarity_info().color;

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
            DrawRectangle(spell_dims.x, spell_dims.y, 3.0f, spell_dims.height, rarity_color);
            spell_dims.x += 3.0f;

            // name bar
            Rectangle name_dims = spell_dims;
            name_dims.height = (int)(spell_dims.height / 3);

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
            exp_dims.height = (int)(stat_bar.height / 3);
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
            DrawTextEx(assets[assets::Macondo], spell_name, name_text_dims, name_font_size, 1.0f, rarity_color);
        }
        EndTextureMode();
        // EndTextureModeMSAA(assets[assets::SpellBookUI, false],
        // assets[assets::SpellBookUI, true]);
    }
}
