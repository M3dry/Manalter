#include "loop.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

#include <format>
#include <raylib.h>
#include <raymath.h>
#include <utility>
#include <variant>

#include "item_drops.hpp"
#include "player.hpp"
#include "rayhacks.hpp"
#include "spell.hpp"
#include "spell_caster.hpp"
#include "ui.hpp"
#include "utility.hpp"

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

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

Arena::Playing::Playing(Keys& keys) {
    keys.unregister_all();
    keys.register_key(KEY_ESCAPE);
    keys.register_key(KEY_N);
    keys.register_key(KEY_M);
    keys.register_key(KEY_B);
    keys.register_key(KEY_ONE);
    keys.register_key(KEY_TWO);
    keys.register_key(KEY_FOUR);
    keys.register_key(KEY_FIVE);
    keys.register_key(KEY_SIX);
    keys.register_key(KEY_SEVEN);
    keys.register_key(KEY_EIGHT);
    keys.register_key(KEY_NINE);
    keys.register_key(KEY_ZERO);
}

Arena::PowerUpSelection::PowerUpSelection(Keys& keys, Vector2 screen)
    : power_ups{PowerUp::random(), PowerUp::random(), PowerUp::random()} {
    keys.unregister_all();
    keys.register_key(KEY_ENTER);
    update_buttons(screen);
}

void Arena::PowerUpSelection::draw([[maybe_unused]] assets::Store& assets, Loop& loop, Arena& arena) {
    for (int i = 0; i < 3; i++) {
        if (selections[i].update(loop.mouse)) {
            arena.player.add_power_up(std::move(power_ups[i]));
            arena.state.emplace<Playing>(loop.keys);
            return;
        }
    }
}

void Arena::PowerUpSelection::update(Arena& arena, Loop& loop) {
    if (loop.screen_updated) {
        update_buttons(loop.screen);
    }

    loop.keys.tick([&](const auto& key) {
        switch (key) {
            case KEY_ENTER:
                arena.state.emplace<Playing>(loop.keys);
                return;
        }
    });
}

void Arena::PowerUpSelection::update_buttons(Vector2 screen) {
    auto height = screen.y * 0.5f;
    auto width = screen.x * 0.15f;
    auto free_space = screen.x - 3 * width;
    auto edge_padding_x = free_space / 2.0f * 0.65f;
    auto gap_between = (free_space - edge_padding_x*2.0f) / 2.0f;
    auto edge_padding_y = (screen.y - height) / 2.0f;

    for (int i = 0; i < 3; i++) {
        auto rec = Rectangle{
            .x = edge_padding_x + width * i + i * gap_between,
            .y = edge_padding_y,
            .width = width,
            .height = height,
        };

        selections.emplace_back(rec, [&, rec](ui::Button::State state) {
            switch (state) {
                using enum ui::Button::State;
                case Normal:
                    power_ups[i].draw(rec);
                    return;
                case Hover:
                    power_ups[i].draw_hover(rec);
                    return;
            }
        });
    }
}

Arena::Paused::Paused(Keys& keys) {
    keys.unregister_all();
    keys.register_key(KEY_ESCAPE);
}

void Arena::Paused::draw([[maybe_unused]] assets::Store& assets, Loop& loop) {
    DrawText("PAUSED", loop.screen.x / 2.0f, loop.screen.y / 2.0f, 30, WHITE);
}

void Arena::Paused::update(Arena& arena, Loop& loop) {
    loop.keys.tick([&](const auto& key) {
        switch (key) {
            case KEY_ESCAPE:
                arena.state.emplace<Playing>(loop.keys);
                return;
        }
    });
}

Arena::Arena(Keys& keys, assets::Store& assets)
    : state(Playing(keys)), player({0.0f, 10.0f, 0.0f}, assets), enemies(30) {
    player.equipped_spells[0] = 0;

    player.exp = 90;
}

void Arena::draw(assets::Store& assets, Loop& loop) {
    if (curr_state<Playing>()) {
        player.update_interpolated_pos(loop.accum_time);
        mouse_xz = mouse_xz_in_world(GetMouseRay(loop.mouse.mouse_pos, player.camera));
    }

    BeginTextureMode(loop.assets[assets::Target, false]);
    ClearBackground(WHITE);

    BeginMode3D(player.camera);

    auto circle =
        shapes::Circle(xz_component(player.position) + Player::visibility_center_offset, Player::visibility_radius);
    auto vs = {
        Vector3Zero(),
        // UP
        (Vector3){-ARENA_WIDTH, 0.0f, 0.0f},
        // DOWN
        (Vector3){ARENA_WIDTH, 0.0f, 0.0f},
        // LEFT
        (Vector3){0.0f, 0.0f, ARENA_HEIGHT},
        // RIGHT
        (Vector3){0.0f, 0.0f, -ARENA_HEIGHT},
        // LEFT-DOWN
        (Vector3){ARENA_WIDTH, 0.0f, ARENA_HEIGHT},
        // RIGHT-DOWN
        (Vector3){ARENA_WIDTH, 0.0f, -ARENA_HEIGHT},
        // LEFT-UP
        (Vector3){-ARENA_WIDTH, 0.0f, ARENA_HEIGHT},
        // RIGHT-UP
        (Vector3){-ARENA_WIDTH, 0.0f, -ARENA_HEIGHT},
    };
    for (const auto& v : vs) {
        DrawPlane(v, (Vector2){ARENA_WIDTH, ARENA_HEIGHT}, GREEN);
        enemies.draw(loop.enemy_models, v, circle);
        item_drops.draw_item_drops(v);
    }
    player.draw_model(assets);

#ifdef DEBUG
    circle.draw_3D(BLUE, 1.0f, Vector2Zero());
    DrawSphere({circle.center.x, 0.0f, circle.center.y}, 3.0f, BLUE);
    caster::draw_hitbox(1.0f);
#endif

    EndMode3D();

#ifdef DEBUG
    item_drops.draw_item_drop_names([camera = player.camera, screen = loop.screen](auto pos) {
        return GetWorldToScreenEx(pos, camera, screen.x, screen.y);
    });
#endif

    EndTextureMode();
    EndTextureModeMSAA(loop.assets[assets::Target, false], loop.assets[assets::Target, true]);

    hud::draw(loop.assets, player, loop.player_stats->spellbook, loop.screen);

    // int textureLoc = GetShaderLocation(fxaa_shader, "texture0");
    // SetShaderValueTexture(fxaa_shader, textureLoc, target.texture);

    BeginDrawing();
    ClearBackground(WHITE);

    // BeginShaderMode(fxaa_shader);
    loop.assets.draw_texture(assets::Target, true, {});
    // EndShaderMode();

    float circle_ui_dim = loop.screen.x * 1 / 8;
    static const float padding = 10;
    SetTextureFilter(loop.assets[assets::CircleUI, true].texture, TEXTURE_FILTER_BILINEAR);
    loop.assets.draw_texture(assets::CircleUI, true,
                             (Rectangle){loop.screen.x - circle_ui_dim - padding,
                                         loop.screen.y - circle_ui_dim - padding, circle_ui_dim, circle_ui_dim});

    float spell_bar_width = loop.screen.x / 4.0f;
    float spell_bar_height = spell_bar_width / 4.0f;
    loop.assets.draw_texture(assets::SpellBarUI, true,
                             (Rectangle){(loop.screen.x - spell_bar_width) / 2.0f, loop.screen.y - spell_bar_height,
                                         spell_bar_width, spell_bar_height});

    if (spellbook_ui) {
        spellbook_ui->update(assets, loop.player_stats->spellbook, loop.mouse, std::nullopt);
    }

    DrawText(std::format("POS: [{}, {}]", player.position.x, player.position.z).c_str(), 10, 10, 20, BLACK);
    DrawText(std::format("ENEMIES: {}", enemies.enemies.data.size()).c_str(), 10, 30, 20, BLACK);
    if (enemies.enemies.data.size() > 0)
            DrawText(std::format("ENEMY1_ANIM: {}", enemies.enemies.data[0].val.anim_curr_frame).c_str(), 10, 50, 20, BLACK);
    if (enemies.enemies.data.size() > 1)
        DrawText(std::format("ENEMY2_ANIM: {}", enemies.enemies.data[1].val.anim_curr_frame).c_str(), 10, 70, 20, BLACK);

    if (curr_state<PowerUpSelection>()) {
        std::get<PowerUpSelection>(state).draw(assets, loop, *this);
    } else if (curr_state<Paused>()) {
        std::get<Paused>(state).draw(assets, loop);
    }

    EndDrawing();
}

void Arena::update(Loop& loop) {
    if (curr_state<PowerUpSelection>()) {
        std::get<PowerUpSelection>(state).update(*this, loop);
        return;
    } else if (curr_state<Paused>()) {
        std::get<Paused>(state).update(*this, loop);
        return;
    }

    static const int movement_keys[] = {KEY_A, KEY_S, KEY_D, KEY_W};
    static const Vector2 movements[] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
    static const float angles[] = {0.0f, 90.0f, 180.0f, 270.0f};

    Vector2 movement = {0, 0};
    Vector2 angle = {0.0f, 0.0f};

    for (int k = 0; k < 4; k++) {
        if (IsKeyDown(movement_keys[k])) {
            movement += movements[k];
            angle.x += (k == 3 && angle.x == 0) ? -90.0f : angles[k];
            angle.y++;
        }
    }
    float length = Vector2Length(movement);
    if (length != 1 && length != 0) movement = Vector2Divide(movement, {length, length});

    static const std::vector<std::pair<int, int>> spell_keys = {
        {KEY_ONE, 0}, {KEY_TWO, 1},   {KEY_THREE, 2}, {KEY_FOUR, 3}, {KEY_FIVE, 4},
        {KEY_SIX, 5}, {KEY_SEVEN, 6}, {KEY_EIGHT, 7}, {KEY_NINE, 8}, {KEY_ZERO, 9},
    };
    loop.keys.tick([&](const auto& key) {
        switch (key) {
            case KEY_ESCAPE:
                state.emplace<Paused>(loop.keys);
                return;
            case KEY_N:
                player.mana -= 1;
                return;
            case KEY_M:
                player.mana += 1;
                return;
            case KEY_B:
                if (spellbook_ui)
                    spellbook_ui = std::nullopt;
                else
                    spellbook_ui = hud::SpellBookUI(loop.player_stats->spellbook, loop.screen);
                return;
        }

        for (const auto& [k, num] : spell_keys) {
            if (k == key) {
                player.cast_equipped(num, (Vector2){player.position.x, player.position.z}, mouse_xz,
                                     loop.player_stats->spellbook, enemies);
                return;
            }
        }
    });

    player.tick((Vector2){movement.x, movement.y}, angle.x / angle.y, loop.player_stats->spellbook);
    auto damage_done = enemies.tick(player.hitbox, loop.enemy_models);
    if (player.health <= damage_done) {
        player.health = 0;
    } else {
        player.health -= damage_done;
        if (player.add_exp(enemies.take_exp())) {
            state.emplace<PowerUpSelection>(loop.keys, loop.screen);
        }
    }

    caster::tick(loop.player_stats->spellbook, enemies, item_drops.item_drops);

    item_drops.pickup(player.hitbox, [&](auto&& arg) {
        using Item = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<Item, Spell>) {
            loop.player_stats->spellbook.emplace_back(std::move(arg));
        }
    });

    if (player.health == 0) {
        loop.scene.emplace<Hub>(loop.keys);
        return;
    }
}

Hub::Hub(Keys& keys) {
    keys.unregister_all();
    keys.register_key(KEY_SPACE);
    keys.register_key(KEY_ESCAPE);
}

void Hub::draw([[maybe_unused]] assets::Store& assets, [[maybe_unused]] Loop& loop) {
    BeginDrawing();

    ClearBackground(BLUE);

    EndDrawing();
}

void Hub::update(Loop& loop) {
    loop.keys.tick([&](auto key) {
        switch (key) {
            case KEY_SPACE:
                loop.scene.emplace<Arena>(loop.keys, loop.assets);
                return;
            case KEY_ESCAPE:
                loop.scene.emplace<Main>(loop.keys, loop.screen);
                loop.player_stats = std::nullopt;
                return;
        }
    });
}

Main::Main(Keys& keys, Vector2 screen) {
    auto play_rec = (Rectangle){
        .x = screen.x * 3.0f / 4.0f,
        .y = screen.y / 3.0f,
        .width = screen.x / 5.0f,
        .height = screen.y / 7.0f,
    };
    play_button.emplace(play_rec, [play_rec](auto state) {
        switch (state) {
            case ui::Button::State::Normal:
                DrawRectangleRec(play_rec, BLACK);
                break;
            case ui::Button::State::Hover:
                DrawRectangleRec(play_rec, RED);
                break;
        }
    });

    keys.unregister_all();
    keys.register_key(KEY_SPACE);
    keys.register_key(KEY_ESCAPE);
}

void Main::draw([[maybe_unused]] assets::Store& assets, Loop& loop) {
    BeginDrawing();
    ClearBackground(WHITE);

    if (play_button->update(loop.mouse)) {
        loop.player_stats = PlayerSave();
        loop.scene.emplace<Hub>(loop.keys);

        loop.player_stats->add_spell_to_spellbook(Spell(spells::FrostNova{}, Rarity::Legendary, 100));
    }

    EndDrawing();
}

void Main::update(Loop& loop) {
    loop.keys.tick([&](auto key) {
        switch (key) {
            case KEY_SPACE: {
                loop.player_stats = PlayerSave();
                loop.scene.emplace<Hub>(loop.keys);

                loop.player_stats->add_spell_to_spellbook(Spell(spells::FrostNova{}, Rarity::Legendary, 100));
                break;
            }
            case KEY_ESCAPE:
                // TEST: should I do some other clean up?
                CloseWindow();
                exit(0);
        }
    });

    if (loop.screen_updated) {
        *this = Main(loop.keys, loop.screen);
    }
}

Loop::Loop(int width, int height)
    : screen((Vector2){(float)width, (float)height}), assets(screen), scene(Main(keys, screen)) {
#ifdef PLATFORM_WEB
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, false, resize_handler);
#endif
};

void Loop::operator()() {
    double current_time = GetTime();
    double delta_time = current_time - prev_time;
    prev_time = current_time;
    accum_time += delta_time;

    PollInputEvents();
    keys.poll();
    mouse.poll();

    std::visit([&](auto&& arg) { arg.draw(assets, *this); }, scene);
    SwapScreenBuffer();

    while (accum_time >= (1.0f / TICKS)) {
        update();
        accum_time -= 1.0f / TICKS;
    }
}

void Loop::update() {
    screen_updated = false;
#ifndef PLATFORM_WEB
    if (IsWindowResized()) {
        screen = (Vector2){(float)GetScreenWidth(), (float)GetScreenHeight()};
        assets.update_target_size(screen);
        screen_updated = true;
    }
#endif

    std::visit([&](auto&& arg) { arg.update(*this); }, scene);
}
