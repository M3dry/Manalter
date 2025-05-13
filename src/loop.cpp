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
#include "font.hpp"
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
    keys.register_key(KEY_THREE);
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

Arena::PowerUpSelection::PowerUpSelection(Loop& loop)
    : power_ups{PowerUp::random(), PowerUp::random(), PowerUp::random()} {
    loop.keys.unregister_all();
    loop.keys.register_key(KEY_ENTER);
    update_buttons(loop.assets, loop.screen);
}

void Arena::PowerUpSelection::draw(Arena& arena, Loop& loop) {
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

Arena::Paused::Paused(Loop& loop) {
    loop.keys.unregister_all();
    loop.keys.register_key(KEY_ESCAPE);

    constexpr auto continue_text = "Continue";
    auto continue_rec = Rectangle{
        .x = loop.screen.x * 0.8f,
        .y = 0.0f,
        .width = loop.screen.x * 0.2f,
        .height = loop.screen.y * 0.1f,
    };
    auto [continue_size, continue_dims] = font_manager::max_font_size(
        font_manager::Alagard, Vector2{continue_rec.width, continue_rec.height}, continue_text);

    continue_rec.width = continue_dims.x;
    continue_rec.height = continue_dims.y;
    auto padding = continue_rec.x * 0.02f;
    continue_rec.x = loop.screen.x - continue_rec.width - padding;
    continue_rec.y = padding;
    continue_button.emplace(continue_rec, [continue_size, continue_rec](auto state) {
        font_manager::draw_text(continue_text, font_manager::Alagard, continue_size,
                                static_cast<float>(continue_size) / 10.0f,
                                state == ui::Button::Hover ? Color{170, 255, 0, 255} : Color{34, 139, 34, 255},
                                Vector2{continue_rec.x, continue_rec.y}, font_manager::Exact);
    });

    constexpr auto quit_text = "Quit";
    auto quit_rec = Rectangle{
        .x = 0.0f,
        .y = loop.screen.y * 0.99f,
        .width = loop.screen.x * 0.2f,
        .height = loop.screen.y * 0.1f,
    };
    auto [quit_size, quit_dims] =
        font_manager::max_font_size(font_manager::Alagard, Vector2{quit_rec.width, quit_rec.height}, quit_text);

    quit_rec.width = quit_dims.x;
    quit_rec.height = quit_dims.y;
    padding = quit_rec.x * 0.02f;
    quit_rec.x = padding;
    quit_rec.y = loop.screen.y - quit_rec.height - padding;
    quit_button.emplace(quit_rec, [quit_size, quit_rec](auto state) {
        font_manager::draw_text(quit_text, font_manager::Alagard, quit_size, static_cast<float>(quit_size) / 10.0f,
                                state == ui::Button::Hover ? Color{210, 43, 43, 255} : Color{136, 8, 8, 255},
                                Vector2{quit_rec.x, quit_rec.y}, font_manager::Exact);
    });
}

void Arena::Paused::draw(Arena& arena, Loop& loop) {
    loop.assets.draw_texture(assets::PauseBackground, Rectangle{
                                                          .x = 0.0f,
                                                          .y = 0.0f,
                                                          .width = loop.screen.x,
                                                          .height = loop.screen.y,
                                                      });

    if (continue_button->update(loop.mouse)) {
        arena.state.emplace<Playing>(loop.keys);
        return;
    }

    if (quit_button->update(loop.mouse)) {
        arena.player.health = 0;
        arena.state.emplace<Playing>(loop.keys);
        return;
    }
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

Arena::Arena(Loop& loop)
    : state(Playing(loop.keys)), player({0.0f, 10.0f, 0.0f}, loop.assets), enemies(100),
      to_next_soul_portal(5.0 * 60.0), soul_portal(std::nullopt) {
    auto arrow_tex = loop.assets[assets::SoulPortalArrow];
    auto arrow_mesh = GenMeshPlane(16, 16, 1, 1);
    soul_portal_arrow = LoadModelFromMesh(arrow_mesh);
    soul_portal_arrow.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = arrow_tex;
    soul_portal_arrow.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
}

Arena::~Arena() {
    UnloadModel(soul_portal_arrow);
}

void Arena::draw(Loop& loop) {
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
    auto floor_tex = loop.assets[assets::Floor];
    for (const auto& v : vs) {
        DrawPlane(v, (Vector2){ARENA_WIDTH, ARENA_HEIGHT}, DARKGRAY);

        enemies.draw(loop.enemy_models, v, circle);

        if (soul_portal) {
            soul_portal->hitbox.draw_3D(BLACK, 1.0f, xz_component(v));
        }
    }
    player.draw_model(loop.assets);

#ifdef DEBUG
    circle.draw_3D(BLUE, 1.0f, Vector2Zero());
    DrawSphere({circle.center.x, 0.0f, circle.center.y}, 3.0f, BLUE);
    caster::draw_hitbox(1.0f);
#endif
    EndMode3D();

    if (playing) effects::update(static_cast<float>(loop.delta_time));
    BeginMode3D(player.camera);
    if (soul_portal && !check_collision(soul_portal->hitbox, xz_component(player.interpolated_position))) {
        auto tex = loop.assets[assets::SoulPortalArrow];

        Vector2 dir_vec;
        dir_vec.x = soul_portal->hitbox.center.x - player.interpolated_position.x;
        dir_vec.y = soul_portal->hitbox.center.y - player.interpolated_position.z;
        dir_vec.x = wrap((dir_vec.x + ARENA_WIDTH / 2.0f), ARENA_WIDTH) - ARENA_WIDTH / 2.0f;
        dir_vec.y = wrap((dir_vec.y + ARENA_HEIGHT / 2.0f), ARENA_HEIGHT) - ARENA_HEIGHT / 2.0f;
        dir_vec = Vector2Normalize(dir_vec);

        auto angle = angle_from_point(-dir_vec, Vector2Zero());

        constexpr auto r = 50.0f;
        auto pos = Vector3{player.interpolated_position.x + r * dir_vec.x, 1.0f,
                           player.interpolated_position.z + r * dir_vec.y};

        DrawModelEx(soul_portal_arrow, pos, Vector3{0.0f, 1.0f, 0.0f}, angle, Vector3{1.0f, 1.0f, 1.0f}, WHITE);
    }

    for (const auto& v : vs) {
        effects::draw(v);
    }
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
        loop.assets.draw_texture(assets::SpellBookBackground, Rectangle{
                                                                  .x = 0.0f,
                                                                  .y = 0.0f,
                                                                  .width = spellbook_ui->spellbook_dims.x,
                                                                  .height = spellbook_ui->spellbook_dims.y,
                                                              });

        const auto& spellbook = loop.player_save->get_spellbook();
        if (auto dropped =
                spellbook_ui->update(loop.mouse, spellbook.size(), curr_state<Playing>(), loop.assets,
                                     std::span(player.equipped_spells.get(), player.unlocked_spell_count), spellbook);
            dropped) {
            auto [spell, pos] = *dropped;

            auto slot = spellbar.dragged(pos, spellbar_dims, player.unlocked_spell_count);
            if (slot != -1) {
                player.equip_spell(static_cast<uint32_t>(spell), static_cast<uint8_t>(slot),
                                   loop.player_save->get_spellbook());
            };
        }
        spellbook_ui->inventory_pane.draw_deffered();
    }

#ifdef DEBUG
    DrawText(std::format("POS: [{}, {}]", player.position.x, player.position.z).c_str(), 10, 10, 20, BLACK);
    DrawText(std::format("ENEMIES: {}", enemies.enemies.data->size()).c_str(), 10, 30, 20, BLACK);
    DrawText(std::format("LVL: {}", player.lvl).c_str(), 10, 50, 20, BLACK);
    DrawText(std::format("UNLOCKED SPELL SPLOTS: {}", player.unlocked_spell_count).c_str(), 10, 70, 20, BLACK);
#endif

    auto time = format_to_time(game_time);
    DrawText(time.c_str(), static_cast<int>(loop.screen.x) - MeasureText(time.c_str(), 20) - 5, 10, 20, BLACK);

    font_manager::draw_text(std::format("Souls: {}", souls).c_str(), font_manager::Alagard, 20, 2, WHITE,
                            Vector2{10.0f, loop.screen.y - 50.0f}, font_manager::Exact);
    font_manager::draw_text(std::format("Claimed souls: {}", loop.player_save->get_souls()).c_str(),
                            font_manager::Alagard, 20, 2, WHITE, Vector2{10.0f, loop.screen.y - 30.0f},
                            font_manager::Exact);

    if (soul_portal && soul_portal->time_remaining >= 58.0) {
        auto text = "A new soul portal has spawned";
        DrawText(text, static_cast<int>(loop.screen.x / 2.0f - static_cast<float>(MeasureText(text, 40)) / 2.0f), 70,
                 40, BLACK);
    }

    if (curr_state<PowerUpSelection>()) {
        std::get<PowerUpSelection>(state).draw(*this, loop);
    } else if (curr_state<Paused>()) {
        std::get<Paused>(state).draw(*this, loop);
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
                state.emplace<Paused>(loop);
                return;
            case KEY_B:
                if (spellbook_ui) {
                    spellbook_ui = std::nullopt;
                } else {
                    auto tile_tex = loop.assets[assets::SpellTileBackground];
                    auto spellbook_tex = loop.assets[assets::SpellBookBackground];

                    auto spellbook_and_tile = spellbook_and_tile_dims(
                        loop.screen,
                        Vector2{static_cast<float>(spellbook_tex.width), static_cast<float>(spellbook_tex.height)},
                        Vector2{static_cast<float>(tile_tex.width), static_cast<float>(tile_tex.height)});
                    auto spellbook = Vector2{spellbook_and_tile.x, spellbook_and_tile.y};
                    auto tile = Vector2{spellbook_and_tile.z, spellbook_and_tile.w};

                    spellbook_ui.emplace(
                        Vector2{spellbook.x, spellbook.y}, loop.screen, Vector2Zero(), spellbook.y * 0.85f,
                        Vector3{spellbook.x * 0.01f, spellbook.x * 0.01f, spellbook.x * 0.02f}, Vector2{tile.x, tile.y},
                        Rectangle{
                            .x = 0.0f,
                            .y = 0.0f,
                            .width = spellbook.x,
                            .height = spellbook.y,
                        },
                        [tile](Vector2 origin, auto state, auto spell_ix, assets::Store& assets, auto equipped,
                               const auto& spellbook) {
                            using State = decltype(state);
                            auto working_area = Rectangle{
                                .x = origin.x,
                                .y = origin.y,
                                .width = tile.x,
                                .height = tile.y,
                            };

                            Color outline = GRAY;
                            if (state == State::Hover) {
                                outline = RED;
                            } else if (state == State::Drag) {
                                outline = RED;
                            } else {
                                for (const auto& ix : equipped) {
                                    if (ix == spell_ix) {
                                        outline = PURPLE;
                                        break;
                                    }
                                }
                            }

                            DrawRectangleRec(working_area, outline);

                            working_area.x += 3;
                            working_area.y += 3;
                            working_area.width -= 6;
                            working_area.height -= 6;

                            spellbook[spell_ix].draw(working_area, assets);
                        },
                        loop.player_save->get_spellbook().size());
                }
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
            if (k != key) continue;

            if (spellbook_ui) {
                auto spell_id = spellbook_ui->inventory_pane.hover(loop.mouse);

                if (spell_id) {
                    player.equip_spell(*spell_id, num, loop.player_save->get_spellbook());
                    return;
                }

                if (loop.mouse.mouse_pos.x < spellbook_ui->spellbook_dims.x &&
                    loop.mouse.mouse_pos.y < spellbook_ui->spellbook_dims.y)
                    return;
            }

            auto spell_id = player.can_cast(num, loop.player_save->get_spellbook());
            loop.player_save->cast_spell(spell_id, (Vector2){player.position.x, player.position.z}, mouse_xz, enemies,
                                         player.mana);

            return;
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
            state.emplace<PowerUpSelection>(loop);
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
        loop.player_save->save();
        loop.player_save->remove_default_spell();
        loop.scene.emplace<Hub>(loop);
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

Hub::Hub(Loop& loop) {
    auto button_rec = Rectangle{
        .x = loop.screen.x / 2.0f - 300 / 2.0f,
        .y = loop.screen.y / 2.0f - 140 / 2.0f,
        .width = 300,
        .height = 140,
    };
    start_button.emplace(button_rec, [&, button_rec](auto state) {
        switch (state) {
            using enum ui::Button::State;
            case Normal: {
                auto [size, dims] = font_manager::max_font_size(
                    font_manager::Alagard, Vector2{button_rec.width, button_rec.height}, "Start game");
                font_manager::draw_text("Start game", font_manager::Alagard, size, static_cast<float>(size) / 10.0f,
                                        WHITE,
                                        Vector2{button_rec.x + button_rec.width / 2.0f - dims.x / 2.0f,
                                                button_rec.y + button_rec.height / 2.0f - dims.y / 2.0f},
                                        font_manager::Exact);
                break;
            }
            case Hover:
                auto [size, dims] = font_manager::max_font_size(
                    font_manager::Alagard, Vector2{button_rec.width, button_rec.height}, "Start game");
                font_manager::draw_text("Start game", font_manager::Alagard, size, static_cast<float>(size) / 10.0f,
                                        BLUE,
                                        Vector2{button_rec.x + button_rec.width / 2.0f - dims.x / 2.0f,
                                                button_rec.y + button_rec.height / 2.0f - dims.y / 2.0f},
                                        font_manager::Exact);
                break;
        }
    });

    auto spellbook_tex = loop.assets[assets::SpellBookBackground];
    auto tile_tex = loop.assets[assets::SpellTileBackground];
    auto spellbook_and_tile = spellbook_and_tile_dims(
        loop.screen, Vector2{static_cast<float>(spellbook_tex.width), static_cast<float>(spellbook_tex.height)},
        Vector2{static_cast<float>(tile_tex.width), static_cast<float>(tile_tex.height)});
    auto tile_dims = Vector2{
        spellbook_and_tile.z,
        spellbook_and_tile.w,
    };

    stash_rec = Rectangle{
        .x = loop.screen.y * 0.1f,
        .y = loop.screen.y * 0.01f,
        .width = tile_dims.x,
        .height = loop.screen.y * 0.9f,
    };

    stash.emplace(
        loop.screen, Vector2{stash_rec.x, stash_rec.y}, stash_rec.height, Vector3{0.0f, 0.0f, stash_rec.height * 0.01f},
        tile_dims,
        Rectangle{
            .x = 0.0f,
            .y = 0.0f,
            .width = loop.screen.x / 2.0f,
            .height = loop.screen.y / 2.0f,
        },
        [tile_dims](Vector2 origin, auto state, auto spell_ix, assets::Store& assets, const auto& spellbook) {
            auto working_area = Rectangle{
                .x = origin.x,
                .y = origin.y,
                .width = tile_dims.x,
                .height = tile_dims.y,
            };

            Color outline = GRAY;
            if (state == decltype(state)::Hover) outline = RED;
            if (state == decltype(state)::Drag) outline = RED;

            DrawRectangleRec(working_area, outline);

            working_area.x += 3;
            working_area.y += 3;
            working_area.width -= 6;
            working_area.height -= 6;

            spellbook[spell_ix].draw(working_area, assets);
        },
        loop.player_save->get_stash().size());

    spellbook_rec = stash_rec;
    spellbook_rec.x = loop.screen.x - stash_rec.x - tile_dims.x;
    spellbook.emplace(
        loop.screen, Vector2{spellbook_rec.x, spellbook_rec.y}, spellbook_rec.height,
        Vector3{0.0f, 0.0f, spellbook_rec.height * 0.01f}, tile_dims,
        Rectangle{
            .x = loop.screen.x / 2.0f,
            .y = 0.0f,
            .width = loop.screen.x / 2.0f,
            .height = loop.screen.y,
        },
        [tile_dims](Vector2 origin, auto state, auto spell_ix, assets::Store& assets, const auto& spellbook) {
            auto working_area = Rectangle{
                .x = origin.x,
                .y = origin.y,
                .width = tile_dims.x,
                .height = tile_dims.y,
            };

            Color outline = GRAY;
            if (state == decltype(state)::Hover) outline = RED;
            if (state == decltype(state)::Drag) outline = RED;

            DrawRectangleRec(working_area, outline);

            working_area.x += 3;
            working_area.y += 3;
            working_area.width -= 6;
            working_area.height -= 6;

            spellbook[spell_ix].draw(working_area, assets);
        },
        loop.player_save->get_stash().size());

    loop.keys.unregister_all();
    loop.keys.register_key(KEY_SPACE);
    loop.keys.register_key(KEY_ESCAPE);
}

void Hub::draw(Loop& loop) {
    BeginDrawing();

    loop.assets.draw_texture(assets::HubBackground, Rectangle{
                                                        .x = 0.0f,
                                                        .y = 0.0f,
                                                        .width = loop.screen.x,
                                                        .height = loop.screen.y,
                                                    });

    if (start_button->update(loop.mouse)) {
        loop.player_save->save();
        loop.player_save->create_default_spell();
        loop.scene.emplace<Arena>(loop);
        EndDrawing();

        return;
    }

    if (auto pos =
            stash->update(loop.mouse, loop.player_save->get_stash().size(), true, loop.assets, loop.player_save->get_stash());
        pos) {
        auto [ix, p] = *pos;

        std::println("DROPEPD {} FROM STASH AT: @[{}, {}]", ix, p.x, p.y);
    }
    if (auto pos = spellbook->update(loop.mouse, loop.player_save->get_spellbook().size(), true, loop.assets,
                                     loop.player_save->get_spellbook());
        pos) {
        auto [ix, p] = *pos;

        std::println("DROPEPD {} FROM SPELLBOOK AT: @[{}, {}]", ix, p.x, p.y);
    }

    stash->draw_deffered();
    spellbook->draw_deffered();

    EndDrawing();
}

void Hub::update(Loop& loop) {
    loop.keys.tick([&](auto key) {
        switch (key) {
            case KEY_SPACE:
                loop.player_save->save();
                loop.player_save->create_default_spell();
                loop.scene.emplace<Arena>(loop);
                return;
            case KEY_ESCAPE:
                loop.player_save->save();
                loop.player_save = std::nullopt;
                loop.scene.emplace<Main>(loop);
                return;
        }
    });
}

Main::Main(Loop& loop) {
    auto play_button_rec = Rectangle{
        .x = loop.screen.x / 1.5f - loop.assets[assets::PlayButton].width / 2.0f,
        .y = loop.screen.y / 2.0f * 0.90f,
        .width = static_cast<float>(loop.assets[assets::PlayButton].width),
        .height = static_cast<float>(loop.assets[assets::PlayButton].height),
    };
    play_button.emplace(play_button_rec, [&, play_button_rec](auto state) {
        switch (state) {
            using enum ui::Button::State;
            case Normal:
                loop.assets.draw_texture(assets::PlayButton, play_button_rec);
                break;
            case Hover:
                loop.assets.draw_texture(assets::PlayButtonHover, play_button_rec);
                break;
        }
    });

    auto exit_button_rec = Rectangle{
        .x = loop.screen.x / 1.5f - loop.assets[assets::PlayButtonHover].width / 2.0f,
        .y = loop.screen.y / 2.0f * 0.90f + loop.assets[assets::PlayButton].height * 1.05f,
        .width = static_cast<float>(loop.assets[assets::PlayButtonHover].width),
        .height = static_cast<float>(loop.assets[assets::PlayButtonHover].height),
    };
    exit_button.emplace(exit_button_rec, [&, exit_button_rec](auto state) {
        /*DrawRectangleRec(exit_button_rec, RED);*/
        switch (state) {
            case ui::Button::State::Normal:
                loop.assets.draw_texture(assets::ExitButton, exit_button_rec);
                break;
            case ui::Button::State::Hover:
                loop.assets.draw_texture(assets::ExitButtonHover, exit_button_rec);
                break;
        }
    });
}

void Main::draw(Loop& loop) {
    BeginDrawing();
    ClearBackground(WHITE);

    auto tex = loop.assets[assets::MainMenu];
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

        loop.scene.emplace<Hub>(loop);
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
        *this = Main(loop);
    }
}

void SplashScreen::draw(Loop& loop) {
    BeginDrawing();
    ClearBackground(WHITE);
    loop.assets.draw_texture(assets::SplashScreen, Rectangle{
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
        loop.scene.emplace<Main>(loop);
        return;
    }
}

void SplashScreen::update([[maybe_unused]] Loop& loop) {
}

Loop::Loop(int width, int height)
    : screen((Vector2){static_cast<float>(width), static_cast<float>(height)}), assets(screen),
      skinning_shader(LoadShader("./assets/skinning.vs.glsl", "./assets/skinning.fs.glsl")), scene(SplashScreen()) {
    font_manager::init();
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

    std::visit([&](auto&& arg) { arg.draw(*this); }, scene);
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
    font_manager::clean();
}
