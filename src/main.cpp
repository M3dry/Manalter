#include <print>
#include <cassert>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <functional>
#include <cmath>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <numbers>
#include <array>
#include <utility>

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include "glad.h"
#include <GLFW/glfw3.h>

#include "spell.hpp"
#include "assets.hpp"

#define TICKS 20

bool is_key_pressed(const std::vector<std::pair<int, bool>>& pressed_keys, bool plus_handled, int key) {
    for (const auto& [k, handled] : pressed_keys) {
        if (!plus_handled && handled) return false;
        if (key == k) return true;
    }

    return false;
}

using Player = struct Player {
    Vector3 prev_position;
    Vector3 position;
    float angle = 0.0f;
    Model model;
    Camera3D camera;
    bool mouse_in_reach = false;
};

using PlayerStats = struct PlayerStats {
    uint32_t max_health;
    uint32_t health;
    uint32_t max_mana;
    uint32_t mana;
    uint16_t lvl;
    // exp collected since level up
    uint32_t exp;
    uint32_t exp_to_next_lvl;
    // 10 max spells
    uint8_t max_spells;
    // uint32_t::max means no spell is equipped
    // otherwise index of spell in spellbook
    std::vector<uint32_t> equipped_spells;
    SpellBook spellbook = {};

    PlayerStats(uint32_t max_health, uint32_t max_mana, uint8_t max_spells)
        : max_health(max_health), max_mana(max_mana), health(max_health), mana(max_mana), lvl(0), exp(0), exp_to_next_lvl(100), max_spells(max_spells), equipped_spells(10, UINT32_MAX) {};

    // Return how much exp is required to level up from `lvl - 1` to `lvl`
    static uint32_t exp_to_lvl(uint16_t lvl) {
        return 100 * std::pow(lvl, 2);
    }

    void add_exp(uint32_t e) {
        exp += e;
        if (exp >= exp_to_next_lvl) {
            lvl++;
            exp -= exp_to_next_lvl;
            exp_to_next_lvl = PlayerStats::exp_to_lvl(lvl + 1);
        }
    }

    std::optional<std::reference_wrapper<const Spell>> get_equipped_spell(int idx) const {
        if (idx >= 10 || idx >= max_spells || equipped_spells[idx] == UINT32_MAX) return {};

        return spellbook[equipped_spells[idx]];
    }

    uint32_t add_spell_to_spellbook(Spell spell) {
        spellbook.push_back(spell);

        return spellbook.size() - 1;
    }

    // -2 - slot out of range
    // -1 - spell isn't inside the spellbook
    // 0 - ok
    int equip_spell(uint32_t spellbook_idx, uint8_t slot_id) {
        if (spellbook.size() <= spellbook_idx) return -1;
        if (max_spells <= slot_id) return -2;

        equipped_spells[slot_id] = spellbook_idx;

        return 0;
    };

    void tick_cooldown() {
        for (int i = 0; i < 10 && i < max_spells; i++) {
            if (equipped_spells[i] == UINT32_MAX) continue;

            Spell& spell = spellbook[equipped_spells[i]];

            if (spell.curr_cooldown == 0) continue;
            spell.curr_cooldown--;
        }
    }

    void cast_equipped(int idx) {
        if (idx >= 10 || idx >= max_spells || equipped_spells[idx] == UINT32_MAX) return;

        Spell& spell = spellbook[equipped_spells[idx]];
        spell.curr_cooldown = spell.get_cooldown();
    }
};

Vector2 mouse_xz_in_world(Ray mouse) {
    const float x = -mouse.position.y/mouse.direction.y;

    assert(mouse.position.y + x*mouse.direction.y == 0);

    return (Vector2){ mouse.position.x + x*mouse.direction.x, mouse.position.z + x*mouse.direction.z };
}

void draw_ui(Assets::Store& assets, const PlayerStats& player_stats, const Vector2& screen) {
    static const uint32_t padding = 10;
    static const uint32_t outer_radius = 1023/2;
    static const Vector2 center = (Vector2){ (float)outer_radius, (float)outer_radius };

    BeginTextureMode(assets[Assets::CircleUI, false]);
        ClearBackground(BLANK);

        DrawCircleV(center, outer_radius, BLACK);

        // EXP
        float angle = 360 * player_stats.exp/player_stats.exp_to_next_lvl;
        DrawCircleV(center, outer_radius - 2*padding, WHITE);
        DrawCircleSector(center, outer_radius - 2*padding, -90.0f, angle - 90.0f, 512, GREEN);

        // HEALTH & MANA
        // https://www.desmos.com/calculator/ltqvnvrd3p
        DrawCircleSector(center, outer_radius - 6*padding, 90.0f, 270.0f, 512, RED);
        DrawCircleSector(center, outer_radius - 6*padding, -90.0f, 90.0f, 512, BLUE);

        float health_s = (float)player_stats.health/(float)player_stats.max_health;
        float mana_s   = (float)player_stats.mana/(float)player_stats.max_mana;
        float health_b = (1 + std::sin(std::numbers::pi_v<double>*health_s - std::numbers::pi_v<double>/2.0f))/2.0f;
        float mana_b   = (1 + std::sin(std::numbers::pi_v<double>*mana_s - std::numbers::pi_v<double>/2.0f))/2.0f;
        int health_height = 2*(outer_radius - 6*padding) * (1 - health_b);
        int mana_height   = 2*(outer_radius - 6*padding) * (1 - mana_b);

        // TODO: clean up this shit
        static const float segments = 100.0f;
        for (int i = 0; i < segments; i++) {
            float health_x = (outer_radius - 6.0f*padding) - (i + 1.0f)*health_height/segments;
            float mana_x = (outer_radius - 6.0f*padding) - (i + 1.0f)*mana_height/segments;
            if (health_x < 0.0f) health_x = (outer_radius - 6.0f*padding) - i*health_height/segments;
            if (mana_x < 0.0f) mana_x = (outer_radius - 6.0f*padding) - i*mana_height/segments;
            float health_width = std::sqrt((outer_radius - 6.0f*padding)*(outer_radius - 6.0f*padding) - health_x*health_x);
            float mana_width = std::sqrt((outer_radius - 6.0f*padding)*(outer_radius - 6.0f*padding) - mana_x*mana_x);

            DrawRectangle(center.x - health_width, 6.0f*padding + (health_height/segments) * i, health_width, health_height/segments + 1, WHITE);
            DrawRectangle(center.x, 6.0f*padding + (mana_height/segments) * i, mana_width, mana_height/segments + 1, WHITE);
        }

        DrawRing(center, outer_radius - 6.2f*padding, outer_radius - 5*padding, 0.0f, 360.0f, 512, BLACK);
        DrawLineEx((Vector2){ center.x, 0.0f }, (Vector2){ center.y, 1022.0f - 6*padding }, 10.0f, BLACK);
    EndTextureMode();
    EndTextureModeMSAA(assets[Assets::CircleUI, false], assets[Assets::CircleUI, true]);

    // Spell Bar
    BeginTextureMode(assets[Assets::SpellBarUI, false]);
        ClearBackground(BLANK);

        DrawRectangle(0, 0, 512, 128, RED);

        const int spell_dim = 128/2; // 2x5 spells

        for (int i = 0; i < 10; i++) {
            int row = i/5;
            int col = i - 5*row;

            if (i >= player_stats.max_spells) {
                assets.draw_texture(Assets::LockedSlot, (Rectangle){ (float)col*spell_dim, (float)spell_dim*row, (float)spell_dim, (float)spell_dim });
                continue;
            }

            if (auto spell_opt = player_stats.get_equipped_spell(i); spell_opt) {
                const Spell& spell = *spell_opt;

                // TODO: rn ignoring the fact that the frame draws over the spell icon
                assets.draw_texture(spell.name, (Rectangle){ (float)col*spell_dim, (float)spell_dim*row, (float)spell_dim, (float)spell_dim });

                BeginBlendMode(BLEND_ADDITIVE);
                    float cooldown_height = spell_dim * spell.curr_cooldown/spell.get_cooldown();
                    DrawRectangle(col*spell_dim, spell_dim*row + (spell_dim - cooldown_height), spell_dim, cooldown_height, { 130, 130, 130, 128 });
                EndBlendMode();

                assets.draw_texture(spell.rarity, (Rectangle){ (float)col*spell_dim, (float)spell_dim*row, (float)spell_dim, (float)spell_dim });
            } else {
                assets.draw_texture(Assets::EmptySpellSlot, (Rectangle){ (float)col*spell_dim, (float)row*spell_dim, (float)spell_dim, (float)spell_dim });
            }
        }

        // RADAR/MAP???
        DrawRectangle(spell_dim*5, 0, 512 - spell_dim*5, 128, GRAY);
    EndTextureMode();
    EndTextureModeMSAA(assets[Assets::SpellBarUI, false], assets[Assets::SpellBarUI, true]);
}

void update(const std::vector<std::pair<int, bool>>& pressed_keys, Assets::Store& assets, Player& player, PlayerStats& player_stats, Vector2& screen, Shader fxaa_shader, int resolutionLoc) {
    static const int keys[] = { KEY_A, KEY_S, KEY_D, KEY_W };
    static const Vector2 movements[] = { { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 } };
    static const float angles[] = { 0.0f, 90.0f, 180.0f, 270.0f };

    Vector2 movement = { 0, 0 };
    Vector2 angle = { 0.0f, 0.0f };

    if (IsWindowResized()) {
        screen = (Vector2){ (float)GetScreenWidth(), (float)GetScreenHeight() };
        SetShaderValue(fxaa_shader, resolutionLoc, &screen, SHADER_UNIFORM_VEC2);

        assets.update_target_size(screen);
    }

    for (int k = 0; k < 4; k++) {
        if (IsKeyDown(keys[k])) {
            movement = Vector2Add(movement, movements[k]);
            angle.x += (k == 3 && angle.x == 0) ? -90.0f : angles[k];
            angle.y++;
        }
    }
    float length = Vector2Length(movement);
    if (length != 1 && length != 0) movement = Vector2Divide(movement, { length, length });

    if (movement.x != 0 || movement.y != 0) {
        auto x = movement.x*5;
        auto z = movement.y*5;

        player.prev_position = player.position;
        player.position.x += x;
        player.position.z += z;
        player.angle = angle.x/angle.y;
    }

    if (is_key_pressed(pressed_keys, false, KEY_N)) {
        player_stats.mana -= 1;
    }
    if (is_key_pressed(pressed_keys, false, KEY_M)) {
        player_stats.mana += 1;
    }

    const std::vector<std::pair<int, int>> spell_keys = {
        { KEY_ONE, 0 },
        { KEY_TWO, 1 },
        { KEY_THREE, 2 },
        { KEY_FOUR, 3 },
        { KEY_FIVE, 4 },
        { KEY_SIX, 5 },
        { KEY_SEVEN, 6 },
        { KEY_EIGHT, 7 },
        { KEY_NINE, 8 },
        { KEY_ZERO, 9 },
    };
    for (const auto& [key, num] : spell_keys) {
        if (is_key_pressed(pressed_keys, false, key)) {
            player_stats.cast_equipped(num);
        }
    }

    player_stats.tick_cooldown();
}

int main() {
    InitWindow(0, 0, "Aetas Magus");
    SetWindowState(FLAG_FULLSCREEN_MODE);
    SetExitKey(KEY_NULL);
    // TODO: dumb fix until I figure out how to change face orientation in blender lol
    // rlDisableBackfaceCulling();
    // DisableCursor();

    Vector2 screen = (Vector2){ (float)GetScreenWidth(), (float)GetScreenHeight() };

    Player player = {
        .prev_position = Vector3Zero(),
        .position = Vector3Zero(),
        .model = LoadModel("./assets/player/player.obj"),
        .camera = {0},
    };
    PlayerStats player_stats(100, 100, 4);
    player_stats.mana = 10;
    player_stats.health = 30;
    player_stats.add_exp(50);

    auto frost_nove_idx = player_stats.add_spell_to_spellbook(Spell(Spell::Frost_Nova, Rarity::Rare, 5));
    auto void_implosion = player_stats.add_spell_to_spellbook(Spell(Spell::Void_Implosion, Rarity::Epic, 20));
    Spell random_spell = Spell::random({Rarity::Uncommon, Rarity::Legendary}, {0, 100});
    auto random_idx = player_stats.add_spell_to_spellbook(random_spell);
    std::println("RANDOM SPELL:\n  name: {}\n  rarity: {}\n  lvl: {}",
                 Spell::icon_map[random_spell.name], Rarity::frames[static_cast<int>(random_spell.rarity)], random_spell.lvl);
    std::println("SPELLBOOK_SIZE: {}", player_stats.spellbook.size());
    player_stats.equip_spell(frost_nove_idx, 0);
    player_stats.equip_spell(random_idx, 2);
    player_stats.equip_spell(void_implosion, 3);

    const Vector3 camera_offset = (Vector3){ 30.0f, 70.0f, 0.0f };
    player.camera.position = (Vector3){ 30.0f, 70.0f, 0.0f };
    player.camera.target = player.position;
    player.camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    player.camera.fovy = 90.0f;
    player.camera.projection = CAMERA_PERSPECTIVE;

    Vector2 mouse_pos = (Vector2){0.0f, 0.0f};

    Assets::Store assets(screen);

    Shader fxaa_shader = LoadShader(0, "./assets/fxaa.glsl");
    int resolutionLoc = GetShaderLocation(fxaa_shader, "resolution");
    SetShaderValue(fxaa_shader, resolutionLoc, &screen, SHADER_UNIFORM_VEC2);

    Shader grass_shader = LoadShader("./assets/grass.vs.glsl", "./assets/grass.fs.glsl");
    Mesh plane_mesh = GenMeshPlane(1000.0f, 1000.0f, 100, 100);
    Model plane_model = LoadModelFromMesh(plane_mesh);
    plane_model.materials[0].shader = grass_shader;

    double mili_accum = 0;
    double time = 0;

    Vector3 interpolated_position = Vector3Zero();

    // TODO: customizable controls
    std::vector<int> registered_keys = { KEY_N, KEY_M, KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE, KEY_SIX, KEY_SEVEN, KEY_EIGHT, KEY_NINE, KEY_ZERO };
    std::vector<std::pair<int, bool>> pressed_keys; // (KEY, if it was handled)
    int key = 0;

    while (!WindowShouldClose()) {
        time = GetTime();
        interpolated_position = Vector3Lerp(player.prev_position, player.position, mili_accum/(1000/TICKS));
        player.camera.target = interpolated_position;
        player.camera.position = Vector3Lerp(player.camera.position, Vector3Add(camera_offset, interpolated_position), 1.0f);

        mouse_pos = mouse_xz_in_world(GetMouseRay(GetMousePosition(), player.camera));
        float distance_to_player = std::abs(Vector2Distance((Vector2){player.position.x, player.position.z}, (Vector2){mouse_pos.x, mouse_pos.y}));
        player.mouse_in_reach = distance_to_player <= 100.0f;

        BeginTextureMode(assets[Assets::Target, false]);
            ClearBackground(WHITE);

            BeginMode3D(player.camera);
                DrawModel(plane_model, Vector3Zero(), 1.0f, WHITE);

                DrawModelEx(player.model, interpolated_position, (Vector3){ 0.0f, 1.0f, 0.0f }, player.angle, (Vector3){ 0.2f, 0.2f, 0.2f }, ORANGE);

                DrawCylinder((Vector3){ mouse_pos.x, 0.0f, mouse_pos.y },
                             5.0f, 5.0f, 1.0f, 128,
                             player.mouse_in_reach ? (Color){ 235, 216, 91, 127 } : (Color){ 237, 175, 164, 127 });
            EndMode3D();
        EndTextureMode();
        EndTextureModeMSAA(assets[Assets::Target, false], assets[Assets::Target, true]);

        // int textureLoc = GetShaderLocation(fxaa_shader, "texture0");
        // SetShaderValueTexture(fxaa_shader, textureLoc, target.texture);

        draw_ui(assets, player_stats, screen);

        BeginDrawing();
            ClearBackground(WHITE);

            // BeginShaderMode(fxaa_shader);
                assets.draw_texture(Assets::Target, true, {});
            // EndShaderMode();

            float circle_ui_dim = screen.x * 1/8;
            static const float padding = 10;
            SetTextureFilter(assets[Assets::CircleUI, true].texture, TEXTURE_FILTER_BILINEAR);
            assets.draw_texture(Assets::CircleUI, true, (Rectangle){ screen.x - circle_ui_dim - padding, screen.y - circle_ui_dim - padding, circle_ui_dim, circle_ui_dim });

            float spell_bar_width = screen.x/4.0f;
            float spell_bar_height = spell_bar_width/4.0f;
            assets.draw_texture(Assets::SpellBarUI, true, (Rectangle){ (screen.x - spell_bar_width)/2.0f, screen.y - spell_bar_height, spell_bar_width, spell_bar_height });
        EndDrawing();
        SwapScreenBuffer();

        PollInputEvents();
        auto [first, last] = std::ranges::remove_if(pressed_keys, [](const auto& pair) {
            return pair.second && IsKeyUp(pair.first);
        });
        pressed_keys.erase(first, last);
        for (const auto& key : registered_keys) {
            if (is_key_pressed(pressed_keys, true, key)) continue;
            if (IsKeyDown(key)) pressed_keys.push_back({ key, false });
        }

        mili_accum += GetTime() - time;
        if (mili_accum*1000 >= (1000/TICKS)) {
            mili_accum = 0;

            update(pressed_keys, assets, player, player_stats, screen, fxaa_shader, resolutionLoc);
            for (auto& [_, handled] : pressed_keys) {
                handled = true;
            }
        }
    }

    UnloadShader(fxaa_shader);
    UnloadShader(grass_shader);
    UnloadModel(player.model);
    UnloadModel(plane_model);

    CloseWindow();

    return 0;
}
