#include "loop.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

#include <raylib.h>
#include <raymath.h>

#include "item_drops.hpp"
#include "rayhacks.hpp"
#include "spell.hpp"
#include "spell_caster.hpp"
#include "ui.hpp"
#include "utility.hpp"

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

bool is_key_pressed(const std::vector<std::pair<int, bool>>& pressed_keys, bool plus_handled, int key) {
    for (const auto& [k, handled] : pressed_keys) {
        if (!plus_handled && handled) return false;
        if (key == k) return true;
    }

    return false;
}

#ifdef PLATFORM_WEB
bool resize_handler(int event_type, const EmscriptenUiEvent* e, void* data_ptr) {
    if (event_type == EMSCRIPTEN_EVENT_RESIZE) {
        Loop* loop = (Loop*)data_ptr;
        loop->screen.x = (float)e->windowInnerWidth;
        loop->screen.y = (float)e->windowInnerHeight;
        loop->assets.update_target_size(loop->screen);
    };

    return true;
}
#endif

Loop::Loop(int width, int height)
    : screen((Vector2){(float)width, (float)height}), player((Vector3){0.0f, 10.0f, 0.0f}),
      player_stats(100, 1, 100, 10, 4), mouse_pos(Vector2Zero()), assets(screen), spellbook_open(false),
      registered_keys({KEY_N, KEY_M, KEY_B, KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE, KEY_SIX, KEY_SEVEN,
                       KEY_EIGHT, KEY_NINE, KEY_ZERO}),
      enemies(100) {
#ifdef PLATFORM_WEB
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, false, resize_handler);
#endif

    auto frost_nove_idx = player_stats.add_spell_to_spellbook(Spell(spells::FrostNova{}, Rarity::Rare, 5));
    auto fire_wall = player_stats.add_spell_to_spellbook(Spell(spells::FireWall{}, Rarity::Epic, 10));
    /*auto void_implosion = player_stats.add_spell_to_spellbook(Spell(Spell::Void_Implosion, Rarity::Epic, 20));*/
    player_stats.equip_spell(frost_nove_idx, 0);
    player_stats.equip_spell(fire_wall, 1);
    /*player_stats.equip_spell(void_implosion, 3);*/

    item_drops.add_item_drop((Vector2){200.0f, 200.0f}, Spell(spells::FireWall{}, Rarity::Epic, 30));
    item_drops.add_item_drop((Vector2){200.0f, 150.0f}, Spell(spells::FrostNova{}, Rarity::Epic, 30));
    /*item_drops.emplace_back((Vector2){200.0f, 100.0f}, Spell(Spell::Falling_Icicle, Rarity::Epic, 30));*/
};

void Loop::operator()() {
    double current_time = GetTime();
    double delta_time = current_time - prev_time;
    prev_time = current_time;
    accum_time += delta_time;

    player.update_interpolated_pos(accum_time);
    mouse_pos = mouse_xz_in_world(GetMouseRay(GetMousePosition(), player.camera));

    BeginTextureMode(assets[assets::Target, false]);
    ClearBackground(WHITE);

    BeginMode3D(player.camera);
    // DrawModel(plane_model, Vector3Zero(), 1.0f, WHITE);
    DrawPlane((Vector3){0.0f, 0.0f, 0.0f}, (Vector2){1000.0f, 1000.0f}, GREEN);

    player.draw_model();
    enemies.draw();
    item_drops.draw_item_drops();

#ifdef DEBUG
    caster::draw_hitbox(1.0f);
#endif

    EndMode3D();

#ifdef DEBUG
    item_drops.draw_item_drop_names([camera = player.camera, screen = this->screen](auto pos) {
        return GetWorldToScreenEx(pos, camera, screen.x, screen.y);
    });
#endif

    EndTextureMode();
    EndTextureModeMSAA(assets[assets::Target, false], assets[assets::Target, true]);

    // int textureLoc = GetShaderLocation(fxaa_shader, "texture0");
    // SetShaderValueTexture(fxaa_shader, textureLoc, target.texture);

    hud::draw(assets, player_stats, screen);

    BeginDrawing();
    ClearBackground(WHITE);

    // BeginShaderMode(fxaa_shader);
    assets.draw_texture(assets::Target, true, {});
    // EndShaderMode();

    float circle_ui_dim = screen.x * 1 / 8;
    static const float padding = 10;
    SetTextureFilter(assets[assets::CircleUI, true].texture, TEXTURE_FILTER_BILINEAR);
    assets.draw_texture(assets::CircleUI, true,
                        (Rectangle){screen.x - circle_ui_dim - padding, screen.y - circle_ui_dim - padding,
                                    circle_ui_dim, circle_ui_dim});

    float spell_bar_width = screen.x / 4.0f;
    float spell_bar_height = spell_bar_width / 4.0f;
    assets.draw_texture(assets::SpellBarUI, true,
                        (Rectangle){(screen.x - spell_bar_width) / 2.0f, screen.y - spell_bar_height, spell_bar_width,
                                    spell_bar_height});

    if (spellbook_open) {
        assets.draw_texture(assets::SpellBookUI, true,
                            (Rectangle){5.0f, 10.0f, SpellBookWidth(screen), SpellBookHeight(screen)});
    }
    EndDrawing();
    SwapScreenBuffer();

    PollInputEvents();
    auto [first, last] =
        std::ranges::remove_if(pressed_keys, [](const auto& pair) { return pair.second && IsKeyUp(pair.first); });
    pressed_keys.erase(first, last);
    for (const auto& key : registered_keys) {
        if (is_key_pressed(pressed_keys, true, key)) continue;
        if (IsKeyDown(key)) pressed_keys.emplace_back(key, false);
    }

    while (accum_time >= (1.0f / TICKS)) {
        update();
        for (auto& [_, handled] : pressed_keys) {
            handled = true;
        }
        accum_time -= 1.0f / TICKS;
    }
}

void Loop::update() {
    static const int keys[] = {KEY_A, KEY_S, KEY_D, KEY_W};
    static const Vector2 movements[] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
    static const float angles[] = {0.0f, 90.0f, 180.0f, 270.0f};

    Vector2 movement = {0, 0};
    Vector2 angle = {0.0f, 0.0f};

#ifndef PLATFORM_WEB
    if (IsWindowResized()) {
        screen = (Vector2){(float)GetScreenWidth(), (float)GetScreenHeight()};
        assets.update_target_size(screen);
    }
#endif

    for (int k = 0; k < 4; k++) {
        if (IsKeyDown(keys[k])) {
            movement += movements[k];
            angle.x += (k == 3 && angle.x == 0) ? -90.0f : angles[k];
            angle.y++;
        }
    }
    float length = Vector2Length(movement);
    if (length != 1 && length != 0) movement = Vector2Divide(movement, {length, length});

    if (is_key_pressed(pressed_keys, false, KEY_N)) {
        player_stats.mana -= 1;
    }
    if (is_key_pressed(pressed_keys, false, KEY_M)) {
        player_stats.mana += 1;
    }

    if (is_key_pressed(pressed_keys, false, KEY_B)) {
        spellbook_open = !spellbook_open;
    }

    const std::vector<std::pair<int, int>> spell_keys = {
        {KEY_ONE, 0}, {KEY_TWO, 1},   {KEY_THREE, 2}, {KEY_FOUR, 3}, {KEY_FIVE, 4},
        {KEY_SIX, 5}, {KEY_SEVEN, 6}, {KEY_EIGHT, 7}, {KEY_NINE, 8}, {KEY_ZERO, 9},
    };
    for (const auto& [key, num] : spell_keys) {
        if (is_key_pressed(pressed_keys, false, key)) {
            player_stats.cast_equipped(num, (Vector2){player.position.x, player.position.z}, mouse_pos);
        }
    }

    player.update((Vector2){movement.x * 5, movement.y * 5}, angle.x / angle.y);
    if (movement.x != 0 || movement.y != 0) {
        enemies.update_target(xz_component(player.position));
    }

    {
        auto [first, last] = std::ranges::remove_if(enemies.enemies, [this](auto& enemy) -> bool {
            enemy.update_target((Vector2){player.position.x, player.position.z});
            if (enemy.health <= 0) {
                return true;
            }

            if (player_stats.health != 0) {
                auto damage = enemy.tick(player.hitbox);

                if (damage >= player_stats.health) player_stats.health = 0;
                else player_stats.health -= damage;
            }

            return false;
        });
        enemies.enemies.erase(first, last);
    }

    if (player_stats.health == 0) {
        /*assert(false && "TODO: death -> main menu");*/
    }

    player_stats.tick();
    caster::tick(player_stats.spellbook, enemies);

    item_drops.pickup(player.hitbox, [&](auto&& arg) {
        using Item = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<Item, Spell>) {
            player_stats.spellbook.emplace_back(std::move(arg));
        }
    });
}
