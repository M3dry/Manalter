#include "loop.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

#include <format>
#include <optional>
#include <print>
#include <raylib.h>
#include <raymath.h>
#include <utility>
#include <variant>

#include "assets.hpp"
#include "effects.hpp"
#include "hitbox.hpp"
#include "item_drops.hpp"
#include "player.hpp"
#include "power_up.hpp"
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

#ifdef DEBUG
    keys.register_key(KEY_P);
    keys.register_key(KEY_O);
    keys.register_key(KEY_I);
#endif
}

Arena::PowerUpSelection::PowerUpSelection(assets::Store& assets, Keys& keys, Vector2 screen)
    : power_ups{PowerUp::random(), PowerUp::random(), PowerUp::random()} {
    keys.unregister_all();
    keys.register_key(KEY_ENTER);
    update_buttons(assets, screen);
}

void Arena::PowerUpSelection::draw([[maybe_unused]] assets::Store& assets, Loop& loop, Arena& arena) {
    for (std::size_t i = 0; i < 3; i++) {
        if (selections[i].update(loop.mouse)) {
            arena.player.add_power_up(std::move(power_ups[i]));
            arena.state.emplace<Playing>(loop.keys);
            return;
        }
    }
}

void Arena::PowerUpSelection::update(Arena& arena, Loop& loop) {
    if (loop.screen_updated) {
        update_buttons(loop.assets, loop.screen);
    }

    loop.keys.tick([&](const auto& key) {
        switch (key) {
            case KEY_ENTER:
                arena.state.emplace<Playing>(loop.keys);
                return;
        }
    });
}

void Arena::PowerUpSelection::update_buttons(assets::Store& assets, Vector2 screen) {
    auto height = screen.y * 0.5f;
    auto width = screen.x * 0.15f;
    auto free_space = screen.x - 3 * width;
    auto edge_padding_x = free_space / 2.0f * 0.65f;
    auto gap_between = (free_space - edge_padding_x * 2.0f) / 2.0f;
    auto edge_padding_y = (screen.y - height) / 2.0f;

    selections.clear();
    for (std::size_t i = 0; i < 3; i++) {
        auto rec = Rectangle{
            .x = edge_padding_x + width * i + i * gap_between,
            .y = edge_padding_y,
            .width = width,
            .height = height,
        };

        selections.emplace_back(rec, [&, i, rec](ui::Button::State button_state) {
            switch (button_state) {
                using enum ui::Button::State;
                case Normal:
                    power_ups[i].draw(assets, rec);
                    return;
                case Hover:
                    power_ups[i].draw_hover(assets, rec);
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
    DrawText("PAUSED", static_cast<int>(loop.screen.x / 2.0f), static_cast<int>(loop.screen.y / 2.0f), 30, WHITE);
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
    : state(Playing(keys)), player({0.0f, 10.0f, 0.0f}, assets), enemies(30), to_next_soul_portal(5.0 * 60.0),
      soul_portal(std::nullopt) {
    player.equipped_spells[0] = 0;

    player.exp = 90;

    auto arrow_tex = assets[assets::SoulPortalArrow];
    auto arrow_mesh = GenMeshPlane(16, 16, 1, 1);
    soul_portal_arrow = LoadModelFromMesh(arrow_mesh);
    soul_portal_arrow.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = arrow_tex;
    soul_portal_arrow.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
}

Arena::~Arena() {
    UnloadModel(soul_portal_arrow);
}

void Arena::draw(assets::Store& assets, Loop& loop) {
    bool playing = curr_state<Playing>();
    if (playing) {
        player.update_interpolated_pos(loop.accum_time);
        mouse_xz = mouse_xz_in_world(GetMouseRay(loop.mouse.mouse_pos, player.camera));

        incr_time(loop.delta_time);
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
    auto floor_tex = assets[assets::Floor];
    for (const auto& v : vs) {
        DrawPlane(v, (Vector2){ARENA_WIDTH, ARENA_HEIGHT}, GREEN);

        enemies.draw(loop.enemy_models, v, circle);
        item_drops.draw_item_drops(v);

        if (soul_portal) {
            soul_portal->hitbox.draw_3D(BLACK, 1.0f, xz_component(v));
        }
    }
    player.draw_model(assets);

#ifdef DEBUG
    circle.draw_3D(BLUE, 1.0f, Vector2Zero());
    DrawSphere({circle.center.x, 0.0f, circle.center.y}, 3.0f, BLUE);
    caster::draw_hitbox(1.0f);
#endif
    EndMode3D();

    if (playing) effects::update(static_cast<float>(loop.delta_time));
    BeginMode3D(player.camera);
    if (soul_portal && !check_collision(soul_portal->hitbox, xz_component(player.interpolated_position))) {
        auto tex = assets[assets::SoulPortalArrow];
        auto angle = angle_from_point(xz_component(player.interpolated_position), soul_portal->hitbox.center);

        static constexpr auto r = 50.0f;
        auto normalized_dir = Vector2Normalize(soul_portal->hitbox.center - xz_component(player.interpolated_position));
        auto pos = Vector3{player.interpolated_position.x, 1.0f, player.interpolated_position.z};
        pos.x += r * normalized_dir.x;
        pos.z += r * normalized_dir.y;

        DrawModelEx(soul_portal_arrow, pos, Vector3{0.0f, 1.0f, 0.0f}, angle, Vector3{1.0f, 1.0f, 1.0f}, WHITE);
    }

    effects::draw();
    EndMode3D();

#ifdef DEBUG
    item_drops.draw_item_drop_names([camera = player.camera, screen = loop.screen](auto pos) {
        return GetWorldToScreenEx(pos, camera, static_cast<int>(screen.x), static_cast<int>(screen.y));
    });
#endif

    EndTextureMode();
    EndTextureModeMSAA(loop.assets[assets::Target, false], loop.assets[assets::Target, true]);

    hud::draw(loop.assets, player, loop.player_save->get_spellbook(), loop.screen);

    // int textureLoc = GetShaderLocation(fxaa_shader, "texture0");
    // SetShaderValueTexture(fxaa_shader, textureLoc, target.texture);

    BeginDrawing();
    ClearBackground(WHITE);

    // BeginShaderMode(fxaa_shader);
    loop.assets.draw_texture(assets::Target, true, {});
    // EndShaderMode();

    float circle_ui_dim = loop.screen.x * 1 / 8;
    const float padding = 10;
    SetTextureFilter(loop.assets[assets::CircleUI, true].texture, TEXTURE_FILTER_BILINEAR);
    loop.assets.draw_texture(assets::CircleUI, true,
                             (Rectangle){loop.screen.x - circle_ui_dim - padding,
                                         loop.screen.y - circle_ui_dim - padding, circle_ui_dim, circle_ui_dim});

    float spell_dim = 96.0f;
    auto spellbar_dims = Vector3{
        .x = loop.screen.x / 2.0f - spell_dim * Player::max_spell_count / 2.0f,
        .y = loop.screen.y - spell_dim,
        .z = spell_dim,
    };
    spellbar.draw(spellbar_dims, loop.assets, loop.player_save->get_spellbook(),
                  std::span(player.equipped_spells.get(), player.unlocked_spell_count));

    if (spellbook_ui) {
        if (auto dropped = spellbook_ui->update(assets, loop.player_save->get_spellbook(),
                                                playing ? loop.mouse : loop.dummy_mouse, std::nullopt);
            dropped) {
            auto [spell, pos] = *dropped;

            auto slot = spellbar.dragged(pos, spellbar_dims, player.unlocked_spell_count);
            if (slot != -1) {
                player.equip_spell(static_cast<uint32_t>(spell), static_cast<uint8_t>(slot),
                                   loop.player_save->get_spellbook());
            };
        }
    }

    DrawText(std::format("POS: [{}, {}]", player.position.x, player.position.z).c_str(), 10, 10, 20, BLACK);
    DrawText(std::format("ENEMIES: {}", enemies.enemies.data.size()).c_str(), 10, 30, 20, BLACK);

    auto time = format_to_time(game_time);
    DrawText(time.c_str(), static_cast<int>(loop.screen.x) - MeasureText(time.c_str(), 20) - 5, 10, 20, BLACK);

    DrawText(std::format("SOULS: {}", souls).c_str(), 10, static_cast<int>(loop.screen.y) - 50, 20, BLACK);
    DrawText(std::format("CLAIMED SOULS: {}", loop.player_save->get_souls()).c_str(), 10,
             static_cast<int>(loop.screen.y) - 30, 20, BLACK);

    if (soul_portal && soul_portal->time_remaining >= 58.0) {
        auto text = "A new soul portal has spawned";
        DrawText(text, static_cast<int>(loop.screen.x / 2.0f - static_cast<float>(MeasureText(text, 40)) / 2.0f), 70,
                 40, BLACK);
    }

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

    const int movement_keys[] = {KEY_A, KEY_S, KEY_D, KEY_W};
    const Vector2 movements[] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
    const float angles[] = {0.0f, 90.0f, 180.0f, 270.0f};

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

    const std::vector<std::pair<int, uint8_t>> spell_keys = {
        {KEY_ONE, 0}, {KEY_TWO, 1},   {KEY_THREE, 2}, {KEY_FOUR, 3}, {KEY_FIVE, 4},
        {KEY_SIX, 5}, {KEY_SEVEN, 6}, {KEY_EIGHT, 7}, {KEY_NINE, 8}, {KEY_ZERO, 9},
    };
    loop.keys.tick([&](const auto& key) {
        switch (key) {
            case KEY_ESCAPE:
                state.emplace<Paused>(loop.keys);
                return;
            case KEY_B:
                if (spellbook_ui)
                    spellbook_ui = std::nullopt;
                else
                    spellbook_ui = hud::SpellBookUI(loop.player_save->get_spellbook(), loop.screen);
                return;
#ifdef DEBUG
            case KEY_P:
                incr_time(100.0);
                break;
            case KEY_O:
                incr_time(10.0);
                break;
            case KEY_I:
                incr_time(1.0);
                break;
#endif
        }

        for (const auto& [k, num] : spell_keys) {
            if (k == key) {
                auto spell_id = player.can_cast(num, loop.player_save->get_spellbook());
                loop.player_save->cast_spell(spell_id, (Vector2){player.position.x, player.position.z}, mouse_xz,
                                             enemies, player.mana);
                return;
            }
        }
    });

    player.tick((Vector2){movement.x, movement.y}, angle.x / angle.y);
    loop.player_save->tick_spellbook();
    auto damage_done = enemies.tick(player.hitbox, loop.enemy_models);
    souls += enemies.take_souls();
    if (player.health <= damage_done) {
        player.health = 0;
    } else {
        player.health -= damage_done;
        if (player.add_exp(enemies.take_exp())) {
            state.emplace<PowerUpSelection>(loop.assets, loop.keys, loop.screen);
        }
    }

    if (soul_portal && check_collision(soul_portal->hitbox, xz_component(player.position))) {
        loop.player_save->add_souls(souls);
        loop.player_save->save();
        souls = 0;
    }

    caster::tick(loop.player_save->get_spellbook(), enemies, item_drops.item_drops);

    item_drops.pickup(player.hitbox, [&](auto&& arg) {
        using Item = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<Item, Spell>) {
            loop.player_save->add_spell_to_spellbook(std::move(arg));
            loop.player_save->save();
        }
    });

    if (player.health == 0) {
        loop.player_save->remove_default_spell();
        loop.player_save->save();
        loop.scene.emplace<Hub>(loop.keys);
        return;
    }
}

void Arena::incr_time(double delta) {
    game_time += delta;
    to_next_soul_portal = std::max(to_next_soul_portal - delta, 0.0);

    if (to_next_soul_portal == 0) {
        if (soul_portal) {
            soul_portal->time_remaining = std::max(soul_portal->time_remaining - delta, 0.0);
        } else {
            soul_portal = SoulPortal{
                .time_remaining = 60.0,
                .hitbox = shapes::Circle({static_cast<float>(GetRandomValue(static_cast<int>(-ARENA_WIDTH / 2.0f),
                                                                            static_cast<int>(ARENA_WIDTH / 2.0f))),
                                          static_cast<float>(GetRandomValue(static_cast<int>(-ARENA_HEIGHT / 2.0f),
                                                                            static_cast<int>(ARENA_HEIGHT / 2.0f)))},
                                         20.0f),
            };
            std::println("SOUL PORTAL SPAWNED @[{}, {}]", soul_portal->hitbox.center.x, soul_portal->hitbox.center.y);
        }

        if (soul_portal->time_remaining == 0) {
            soul_portal = std::nullopt;
            to_next_soul_portal = 4.0 * 60.0;
        }
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
                loop.player_save->save();
                loop.player_save->create_default_spell();
                loop.scene.emplace<Arena>(loop.keys, loop.assets);
                return;
            case KEY_ESCAPE:
                loop.player_save->save();
                loop.player_save = std::nullopt;
                loop.scene.emplace<Main>(loop.assets, loop.keys, loop.screen);
                return;
        }
    });
}

Main::Main(assets::Store& assets, Keys& keys, Vector2 screen) {
    auto play_button_rec = Rectangle{
        .x = screen.x / 1.5f - assets[assets::PlayButton].width / 2.0f,
        .y = screen.y / 2.0f * 0.90f,
        .width = static_cast<float>(assets[assets::PlayButton].width),
        .height = static_cast<float>(assets[assets::PlayButton].height),
    };
    play_button.emplace(play_button_rec, [&, play_button_rec](auto state) {
        /*DrawRectangleRec(play_button_rec, BLUE);*/
        switch (state) {
            case ui::Button::State::Normal:
                assets.draw_texture(assets::PlayButton, play_button_rec);
                break;
            case ui::Button::State::Hover:
                assets.draw_texture(assets::PlayButtonHover, play_button_rec);
                break;
        }
    });

    auto exit_button_rec = Rectangle{
        .x = screen.x / 1.5f - assets[assets::PlayButtonHover].width / 2.0f,
        .y = screen.y / 2.0f * 0.90f + assets[assets::PlayButton].height * 1.05f,
        .width = static_cast<float>(assets[assets::PlayButtonHover].width),
        .height = static_cast<float>(assets[assets::PlayButtonHover].height),
    };
    exit_button.emplace(exit_button_rec, [&, exit_button_rec](auto state) {
        /*DrawRectangleRec(exit_button_rec, RED);*/
        switch (state) {
            case ui::Button::State::Normal:
                assets.draw_texture(assets::ExitButton, exit_button_rec);
                break;
            case ui::Button::State::Hover:
                assets.draw_texture(assets::ExitButtonHover, exit_button_rec);
                break;
        }
    });
}

void Main::draw([[maybe_unused]] assets::Store& assets, Loop& loop) {
    BeginDrawing();
    ClearBackground(WHITE);

    auto tex = assets[assets::MainMenu];
    auto screen_ratio = loop.screen.x / loop.screen.y;
    DrawTexturePro(tex,
                   Rectangle{
                       .x = 100.0f,
                       .y = 300.0f,
                       .width = screen_ratio * (loop.screen.y - 300.0f),
                       .height = loop.screen.y - 300.0f,
                   },
                   Rectangle{
                       .x = 0.0f,
                       .y = 0.0f,
                       .width = loop.screen.x,
                       .height = loop.screen.y,
                   },
                   Vector2Zero(), 0.0f, WHITE);

    if (play_button->update(loop.mouse)) {
        loop.player_save = PlayerSave();
        loop.player_save->load_save();

        loop.scene.emplace<Hub>(loop.keys);
        return;
    }

    if (exit_button->update(loop.mouse)) {
        EndDrawing();
        CloseWindow();
        exit(0);
    }

    EndDrawing();
}

void Main::update(Loop& loop) {
    if (loop.screen_updated) {
        *this = Main(loop.assets, loop.keys, loop.screen);
    }
}

void SplashScreen::draw(assets::Store& assets, Loop& loop) {
    BeginDrawing();
    ClearBackground(WHITE);
    assets.draw_texture(assets::SplashScreen, Rectangle{
                                                  .x = 0.0f,
                                                  .y = 0.0f,
                                                  .width = loop.screen.x,
                                                  .height = loop.screen.y,
                                              });
    EndDrawing();

    // NOTE: This is a very ugly hack, the key handling isn't in `update` cuz PollInputEvents gets called every frame.
    // There isn't a nice way to poll all the keys that changed state without hacking raylib or making a custom glfw
    // callback handler.
    auto key = 0;
    while ((key = GetKeyPressed()) != 0) {
        loop.keys.handled_key(key);
        loop.scene.emplace<Main>(loop.assets, loop.keys, loop.screen);
        return;
    }
}

void SplashScreen::update(Loop& loop) {
}

Loop::Loop(int width, int height)
    : screen((Vector2){static_cast<float>(width), static_cast<float>(height)}), assets(screen),
      skinning_shader(LoadShader("./assets/skinning.vs.glsl", "./assets/skinning.fs.glsl")), scene(SplashScreen()) {
#ifdef PLATFORM_WEB
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, false, resize_handler);
#endif
    enemy_models.add_shader(skinning_shader);
};

void Loop::operator()() {
    double current_time = GetTime();
    delta_time = current_time - prev_time;
    prev_time = current_time;
    accum_time += delta_time;

    PollInputEvents();
    keys.poll();
    mouse.poll();

    std::visit([&](auto&& arg) { arg.draw(assets, *this); }, scene);
    SwapScreenBuffer();

    while (accum_time >= (1.0 / TICKS)) {
        update();
        accum_time -= 1.0 / TICKS;
    }
}

void Loop::update() {
    screen_updated = false;
#ifndef PLATFORM_WEB
    if (IsWindowResized()) {
        screen = (Vector2){static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())};
        assets.update_target_size(screen);
        screen_updated = true;
    }
#endif

    std::visit([&](auto&& arg) { arg.update(*this); }, scene);
}

Loop::~Loop() {
    UnloadShader(skinning_shader);
    effects::clean();
}
