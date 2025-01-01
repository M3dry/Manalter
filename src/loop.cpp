#include "loop.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <numbers>
#include <ranges>
#include <string>

#include <raylib.h>
#include <raymath.h>

#include "rayhacks.hpp"
#include "spell_caster.hpp"
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

const Vector3 Player::camera_offset = (Vector3){30.0f, 70.0f, 0.0f};
const float Player::model_scale = 0.2f;

Player::Player(Vector3 position)
    : prev_position(position), position(position), interpolated_position(position),
      model(LoadModel("./assets/player/player.obj")) {
    camera.position = camera_offset;
    camera.target = position;
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 90.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    BoundingBox mesh_bb = GetMeshBoundingBox(model.meshes[0]);
    Vector3 min = Vector3Scale(mesh_bb.min, model_scale);
    Vector3 max = Vector3Scale(mesh_bb.max, model_scale);

    hitbox =
        shapes::Polygon((Vector2){max.x + min.x, max.z + min.z}, {(Vector2){min.x, max.z}, (Vector2){min.x, min.z},
                                                                  (Vector2){max.x, min.z}, (Vector2){max.x, max.z}});
}

void Player::update_interpolated_pos(double accum_time) {
    interpolated_position = Vector3Lerp(prev_position, position, accum_time * TICKS);
    prev_position = interpolated_position;
    camera.target = interpolated_position;
    camera.position = Vector3Lerp(camera.position, Vector3Add(camera_offset, interpolated_position), 1.0f);
}

void Player::update_position(Vector2 movement, float new_angle) {
    prev_position = position;
    position.x += movement.x;
    position.z += movement.y;
    hitbox->rotate(angle - new_angle);
    angle = new_angle;

    hitbox->update(movement);
}

void Player::draw_model() const {
    DrawModelEx(model, interpolated_position, (Vector3){0.0f, 1.0f, 0.0f}, angle,
                (Vector3){model_scale, model_scale, model_scale}, ORANGE);

#ifdef DEBUG
    DrawSphere((Vector3){hitbox->center->x, 1.0f, hitbox->center->y}, 1.0f, BLUE);
    hitbox->draw_lines_3D(RED, 1.0f);
#endif
}

Player::~Player() {
    UnloadModel(model);
}

PlayerStats::PlayerStats(uint32_t max_health, uint32_t health_regen, uint32_t max_mana, uint32_t mana_regen,
                         uint8_t max_spells)
    : max_health(max_health), health(max_health), health_regen(health_regen), max_mana(max_mana), mana(max_mana),
      mana_regen(mana_regen), lvl(0), exp(0), exp_to_next_lvl(100), max_spells(max_spells),
      equipped_spells(10, UINT32_MAX) {};

uint32_t PlayerStats::exp_to_lvl(uint16_t lvl) {
    return 100 * std::pow(lvl, 2);
}

void PlayerStats::add_exp(uint32_t e) {
    exp += e;
    if (exp >= exp_to_next_lvl) {
        lvl++;
        exp -= exp_to_next_lvl;
        exp_to_next_lvl = PlayerStats::exp_to_lvl(lvl + 1);
    }
}

std::optional<std::reference_wrapper<const Spell>> PlayerStats::get_equipped_spell(int idx) const {
    if (idx >= 10 || idx >= max_spells || equipped_spells[idx] == UINT32_MAX) return {};

    return spellbook[equipped_spells[idx]];
}

uint32_t PlayerStats::add_spell_to_spellbook(Spell&& spell) {
    spellbook.emplace_back(std::move(spell));

    return spellbook.size() - 1;
}

int PlayerStats::equip_spell(uint32_t spellbook_idx, uint8_t slot_id) {
    if (spellbook.size() <= spellbook_idx) return -1;
    if (max_spells <= slot_id) return -2;

    equipped_spells[slot_id] = spellbook_idx;

    return 0;
}

void PlayerStats::tick() {
    if (tick_counter == TICKS) {
        tick_counter = 0;
        health += health_regen;
        mana += mana_regen;

        if (health > max_health) health = max_health;
        if (mana > max_mana) mana = max_mana;
    }

    for (int i = 0; i < 10 && i < max_spells; i++) {
        if (equipped_spells[i] == UINT32_MAX) continue;

        Spell& spell = spellbook[equipped_spells[i]];

        if (spell.curr_cooldown == 0) continue;
        spell.curr_cooldown--;
    }

    tick_counter++;
}

void PlayerStats::cast_equipped(int idx, const Vector2& player_position, const Vector2& mouse_pos) {
    if (idx >= 10 || idx >= max_spells || equipped_spells[idx] == UINT32_MAX) return;

    auto spell_id = equipped_spells[idx];
    Spell& spell = spellbook[spell_id];
    if (mana < spell.manacost) return;

    mana -= spell.manacost;
    spell.curr_cooldown = spell.get_cooldown();

    caster::cast(spell_id, spell, player_position, mouse_pos);
}

const float ItemDrop::hitbox_radius = 5.0f;

ItemDrop::ItemDrop(Vector2 center, Spell&& spell)
    : type(Type::Spell), hitbox(center, hitbox_radius), item(std::move(spell)) {
}

void ItemDrop::draw_name(std::function<Vector2(Vector3)> to_screen_coords) const {
    Vector2 pos = to_screen_coords((Vector3){hitbox.center.x, 0.0f, hitbox.center.y});
    auto name = get_name().data();
    DrawText(name, pos.x, pos.y, 20, WHITE);
}

std::string_view ItemDrop::get_name() const {
    switch (type) {
        case Type::Spell:
            return std::get<Spell>(item).get_name();
        default:
            assert(false);
    }
}

Vector2 mouse_xz_in_world(Ray mouse) {
    const float x = -mouse.position.y / mouse.direction.y;

    return (Vector2){mouse.position.x + x * mouse.direction.x, mouse.position.z + x * mouse.direction.z};
}

void draw_ui(assets::Store& assets, const PlayerStats& player_stats, const Vector2& screen) {
    static const uint32_t padding = 10;
    static const uint32_t outer_radius = 1023 / 2;
    static const Vector2 center = (Vector2){(float)outer_radius, (float)outer_radius};

    BeginTextureMode(assets[assets::CircleUI, false]);
    ClearBackground(BLANK);

    DrawCircleV(center, outer_radius, BLACK);

    // EXP
    float angle = 360.0f * (float)player_stats.exp / player_stats.exp_to_next_lvl;
    DrawCircleV(center, outer_radius - 2 * padding, WHITE);
    DrawCircleSector(center, outer_radius - 2 * padding, -90.0f, angle - 90.0f, 512, GREEN);

    // HEALTH & MANA
    // https://www.desmos.com/calculator/ltqvnvrd3p
    DrawCircleSector(center, outer_radius - 6 * padding, 90.0f, 270.0f, 512, RED);
    DrawCircleSector(center, outer_radius - 6 * padding, -90.0f, 90.0f, 512, BLUE);

    float health_s = (float)player_stats.health / (float)player_stats.max_health;
    float mana_s = (float)player_stats.mana / (float)player_stats.max_mana;
    float health_b = (1 + std::sin(std::numbers::pi_v<double> * health_s - std::numbers::pi_v<double> / 2.0f)) / 2.0f;
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
        DrawRectangle(center.x, 6.0f * padding + (mana_height / segments) * i, mana_width, mana_height / segments + 1,
                      WHITE);
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

        if (i >= player_stats.max_spells) {
            assets.draw_texture(assets::LockedSlot, (Rectangle){(float)col * spell_dim, (float)spell_dim * row,
                                                                (float)spell_dim, (float)spell_dim});
            continue;
        }

        if (auto spell_opt = player_stats.get_equipped_spell(i); spell_opt) {
            const Spell& spell = *spell_opt;

            // TODO: rn ignoring the fact that the frame draws over the spell
            // icon
            assets.draw_texture(spell.name, (Rectangle){(float)col * spell_dim, (float)spell_dim * row,
                                                        (float)spell_dim, (float)spell_dim});

            BeginBlendMode(BLEND_ADDITIVE);
            float cooldown_height = spell_dim * (float)spell.curr_cooldown / spell.get_cooldown();
            DrawRectangle(col * spell_dim, spell_dim * row + (spell_dim - cooldown_height), spell_dim, cooldown_height,
                          {130, 130, 130, 128});
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
    auto& spellbook = player_stats.spellbook;
    int total_spells = spellbook.size();
    int page_size = std::min((int)(spellbook_dims.y / spell_height - 1), total_spells);
    for (i = 0; i < page_size; i++) {
        auto rarity_color = spellbook[i].get_rarity_color();

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
        assets.draw_texture(spellbook[i].name,
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

        auto spell_name = spellbook[i].get_name();
        auto [name_font_size, name_text_dims] =
            max_font_size(assets[assets::Macondo], 1.0f, (Vector2){name_dims.width, name_dims.height}, spell_name);
        name_text_dims.x = name_dims.x + name_dims.width / 2.0f - name_text_dims.x / 2.0f;
        name_text_dims.y = name_dims.y;
        DrawTextEx(assets[assets::Macondo], spell_name.data(), name_text_dims, name_font_size, 1.0f, rarity_color);
    }
    EndTextureMode();
    // EndTextureModeMSAA(assets[assets::SpellBookUI, false],
    // assets[assets::SpellBookUI, true]);
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
                       KEY_EIGHT, KEY_NINE, KEY_ZERO}) {
#ifdef PLATFORM_WEB
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, false, resize_handler);
#endif

    auto frost_nove_idx = player_stats.add_spell_to_spellbook(Spell(Spell::Frost_Nova, Rarity::Rare, 5));
    auto fire_wall = player_stats.add_spell_to_spellbook(Spell(Spell::Fire_Wall, Rarity::Epic, 10));
    auto void_implosion = player_stats.add_spell_to_spellbook(Spell(Spell::Void_Implosion, Rarity::Epic, 20));
    player_stats.equip_spell(frost_nove_idx, 0);
    player_stats.equip_spell(fire_wall, 1);
    player_stats.equip_spell(void_implosion, 3);

    item_drops.emplace_back((Vector2){200.0f, 200.0f}, Spell(Spell::Fire_Wall, Rarity::Epic, 30));
    item_drops.emplace_back((Vector2){200.0f, 150.0f}, Spell(Spell::Frost_Nova, Rarity::Epic, 30));
    item_drops.emplace_back((Vector2){200.0f, 100.0f}, Spell(Spell::Falling_Icicle, Rarity::Epic, 30));
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

    for (const auto& item_drop : item_drops) {
        DrawCircle3D((Vector3){item_drop.hitbox.center.x, 1.0f, item_drop.hitbox.center.y}, item_drop.hitbox.radius,
                     (Vector3){1.0f, 0.0f, 0.0f}, 90.0f, RED);
    }

#ifdef DEBUG
    caster::draw_hitbox(1.0f);
#endif
    EndMode3D();

    for (const auto& item_drop : item_drops) {
        // FIX: capture by value
        item_drop.draw_name([camera = player.camera, screen = this->screen](auto pos) {
            return GetWorldToScreenEx(pos, camera, screen.x, screen.y);
        });
    }
    EndTextureMode();
    EndTextureModeMSAA(assets[assets::Target, false], assets[assets::Target, true]);

    // int textureLoc = GetShaderLocation(fxaa_shader, "texture0");
    // SetShaderValueTexture(fxaa_shader, textureLoc, target.texture);

    draw_ui(assets, player_stats, screen);

    BeginDrawing();
    ClearBackground(WHITE);

    // BeginShaderMode(fxaa_shader);
    assets.draw_texture(assets::Target, true, {});
    // EndShaderMode();
    DrawText(("POS: [" + std::to_string(player.position.x) + ", " + std::to_string(player.position.z) + "]").c_str(),
             10, 10, 20, BLACK);
    DrawText(("INTER_POS: [" + std::to_string(player.interpolated_position.x) + ", " +
              std::to_string(player.interpolated_position.z) + "]")
                 .c_str(),
             10, 30, 20, BLACK);
    DrawText(("ANGLE: " + std::to_string(player.angle)).c_str(), 10, 50, 20, BLACK);

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

    if (movement.x != 0 || movement.y != 0) {
        player.update_position((Vector2){movement.x * 5, movement.y * 5}, angle.x / angle.y);
    }

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

    player_stats.tick();
    caster::tick(player_stats.spellbook);

    std::vector<ItemDrop>::iterator it = item_drops.begin();
    int i = 0;
    while (it != item_drops.end()) {
        if (check_collision(*player.hitbox, item_drops[i].hitbox)) {
            switch (item_drops[i].type) {
                case ItemDrop::Type::Spell:
                    player_stats.spellbook.emplace_back(item_drops[i].move_item<Spell>());
                    break;
            }
            it = item_drops.erase(it);
        } else
            it++;

        i++;
    }
}
