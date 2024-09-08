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

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#define TICKS 20

// very generic names, TODO: maybe come up with better ones
enum struct Rarity {
    Common,
    Uncommon,
    Rare,
    Epic,
    Legendary,
};

using Spell = struct Spell {
    enum Name {
        Fire_Wall,
        Falling_Icicle,
        Lightning_Strike,
        Frost_Nova,
        Void_Implosion,
        Mana_Detonation,
    };

    enum Type {
        AOE,
        AtMouse,
        AtPlayer,
        Fire,
        Ice,
        Lightning,
        DarkMagic,
    };

    static const std::unordered_map<Name, std::unordered_set<Type>> type_map;
    static const std::string icon_path;
    static const std::unordered_map<Name, std::string> icon_map; // just file name, path is assumed at ./assets/spell-icons/
    static const std::unordered_map<Rarity, float> rarity_multiplier;

    Name name;
    Rarity rarity;
    uint16_t lvl;
    // exp collected since level up
    uint32_t exp;

    Spell(Name name, Rarity rarity, uint16_t lvl)
        : name(name), rarity(rarity), lvl(lvl), exp(0) {};
};

const std::unordered_map<Spell::Name, std::unordered_set<Spell::Type>> Spell::type_map = {
    { Spell::Fire_Wall, { Spell::AOE, Spell::AtMouse, Spell::Fire } },
    { Spell::Falling_Icicle, { Spell::AtMouse, Spell::Ice } },
    { Spell::Lightning_Strike, { Spell::AtMouse, Spell::Lightning } },
    { Spell::Frost_Nova, { Spell::AOE,  Spell::AtMouse, Spell::Ice } },
    { Spell::Void_Implosion, { Spell::AOE, Spell::AtMouse, Spell::DarkMagic } },
    { Spell::Mana_Detonation, { Spell::AOE, Spell::AtPlayer, Spell::DarkMagic } },
};

const std::string Spell::icon_path = "./assets/spell-icons";
const std::unordered_map<Spell::Name, std::string> Spell::icon_map = {
    { Spell::Fire_Wall, "fire-wall.png" },
    { Spell::Falling_Icicle, "falling-icicle.png" },
    { Spell::Lightning_Strike, "lightning-strike.png" },
    { Spell::Frost_Nova, "frost-nova.png" },
    { Spell::Void_Implosion, "void-implosion.png" },
    { Spell::Mana_Detonation, "mana-detonation.png" },
};

const std::unordered_map<Rarity, float> Spell::rarity_multiplier = {
    { Rarity::Common, 1.0f },
    { Rarity::Uncommon, 1.1f },
    { Rarity::Rare, 1.3f },
    { Rarity::Epic, 1.6f },
    { Rarity::Legendary, 2.0f },
};

using SpellBook = std::vector<Spell>;

using Player = struct Player {
    Vector3 position;
    float angle = 0.0f;
    Model model;
    Camera3D camera;
    bool mouse_in_reach = false;
    SpellBook spellbook = {};

    uint32_t add_spell(Spell spell) {
        spellbook.push_back(spell);

        return spellbook.size() - 1;
    }
};

using PlayerStats = struct PlayerStats {
    uint32_t max_health;
    uint32_t health;
    uint32_t max_mana;
    uint32_t mana;
    // realistically only like 10 max spells
    uint8_t max_spells;
    // uint32_t::max means no spell is equipped
    // otherwise index of spell in spellbook
    std::vector<uint32_t> equipped_spells;

    PlayerStats(uint32_t max_health, uint32_t max_mana, uint8_t max_spells)
        : max_health(max_health), max_mana(max_mana), health(max_health), mana(max_mana), max_spells(max_spells), equipped_spells(max_spells, UINT32_MAX) {};

    // -2 - slot out of range
    // -1 - spell isn't inside the spellbook
    // 0 - ok
    int equip_spell(const SpellBook& spellbook, uint32_t spellbook_idx, uint8_t slot_id) {
        if (spellbook.size() >= spellbook_idx) return -1;
        if (max_spells >= slot_id) return -2;

        equipped_spells[slot_id] = spellbook_idx;

        return 0;
    };
};

Texture2D make_texture(Vector2 img_size, Color img_color, std::function<void(Image)> on_image) {
    Image img = GenImageColor(img_size.x, img_size.y, img_color);
    on_image(img);
    Texture2D texture = LoadTextureFromImage(img);
    GenTextureMipmaps(&texture);
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    UnloadImage(img);

    return texture;
}

class Textures {
  public:
    enum GeneralId {
        // Circle,
    };

    Textures() : general_map({}), spell_map({}) {
        add_spell_textures();
    };
    Textures(const Textures&) = delete;

    ~Textures() {
        for (auto& [_, val] : general_map) {
            UnloadTexture(val);
        }

        for (auto& [_, val] : spell_map) {
            UnloadTexture(val);
        }
    };

    Texture2D operator [](GeneralId id) {
        return general_map[id];
    }

    Texture2D operator [](Spell::Name name) {
        return spell_map[name];
    }

  private:
    std::unordered_map<GeneralId, Texture2D> general_map;
    std::unordered_map<Spell::Name, Texture2D> spell_map;

    void add_spell_textures() {
        for (const auto& [k, path] : Spell::icon_map) {
            Image img = LoadImage((Spell::icon_path + "/" + path).c_str());
            spell_map[k] = LoadTextureFromImage(img);
            UnloadImage(img);
        }
    };
};

Vector2 mouse_xz_in_world(Ray mouse) {
    const float x = -mouse.position.y/mouse.direction.y;

    assert(mouse.position.y + x*mouse.direction.y == 0);

    return (Vector2){ mouse.position.x + x*mouse.direction.x, mouse.position.z + x*mouse.direction.z };
}

void draw_circle_texture(Texture2D texture, Vector2 center, float radius, Color color) {
    DrawTextureEx(texture, (Vector2){ center.x - radius, center.y - radius }, 0.0f, 0.001f * radius, color);
}

void draw_ui(Textures& textures, const PlayerStats& player_stats, const Vector2& screen) {
    // HP & mana
    static const uint32_t padding = 10;
    uint32_t start_x = screen.x * 7/8;
    uint32_t outer_radius = (screen.x - start_x)/2;
    Vector2 center = (Vector2){ (float)(start_x + outer_radius - padding), screen.y - outer_radius - padding };

    // draw_circle_texture(textures[Textures::Circle], center, outer_radius + padding/3.0f, BLACK);
    // draw_circle_texture(textures[Textures::Circle], center, outer_radius, WHITE);
    DrawCircleV(center, outer_radius + padding/3.0f, BLACK);
    DrawCircleV(center, outer_radius, WHITE);

    // 0 - 360 range, player_stats.max_mana != 0
    float angle = 360 * player_stats.mana/player_stats.max_mana;
    DrawCircleSector(center, outer_radius, -90.0f, angle - 90.0f, 512, BLUE);

    // draw_circle_texture(textures[Textures::Circle], center, outer_radius - 7.0f/3.0f*padding, BLACK);
    // draw_circle_texture(textures[Textures::Circle], center, outer_radius - 8.0f/3.0f*padding, RED);
    DrawCircleV(center, outer_radius - 7.0f/3.0f*padding, BLACK);
    DrawCircleV(center, outer_radius - 8.0f/3.0f*padding, RED);

    auto mana_text = TextFormat("%d/%d", player_stats.mana, player_stats.max_mana);
    DrawText(mana_text, center.x - MeasureText(mana_text, 20)/2, center.y - outer_radius + 5.0f/3.0f*padding - 10, 20, BLACK);

    auto health_text = TextFormat("%d/%d", player_stats.health, player_stats.max_health);
    DrawText(health_text, center.x - MeasureText(health_text, 30)/2, center.y - 15, 30, WHITE);


    // Spells
    int width = screen.x * 2.0f/5.0f;
    int height = screen.y/20.0f;
    Rectangle spells_rec = (Rectangle){ screen.x/2 - width/2, screen.y - height, width, height };
    DrawRectangleRounded({ spells_rec.x, spells_rec.y, spells_rec.width, 2*spells_rec.height }, 0.5f, 512, BLACK);
    DrawRectangleRounded({ spells_rec.x + padding/3.0f, spells_rec.y + padding/3.0f, spells_rec.width - padding*2.0f/3.0f, 2*spells_rec.height }, 0.45f, 512, WHITE);
}

void update(Player& player, PlayerStats& player_stats, Vector2& mouse_pos, Vector2& screen, Shader fxaa_shader, int resolutionLoc, RenderTexture2D& target) {
    static std::vector<int> pressed_keys = {};

    auto [first, last] = std::ranges::remove_if(pressed_keys, [](int key) {
        return IsKeyReleased(key);
    });
    pressed_keys.erase(first, last);

    static const int keys[] = { KEY_A, KEY_S, KEY_D, KEY_W };
    static const Vector2 movements[] = { { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 } };
    static const float angles[] = { 0.0f, 90.0f, 180.0f, 270.0f };

    Vector2 movement = { 0, 0 };
    Vector2 angle = { 0.0f, 0.0f };

    if (IsWindowResized()) {
        screen = (Vector2){ (float)GetScreenWidth(), (float)GetScreenHeight() };
        SetShaderValue(fxaa_shader, resolutionLoc, &screen, SHADER_UNIFORM_VEC2);

        UnloadRenderTexture(target);
        target = LoadRenderTexture(screen.x, screen.y);
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
        auto x = movement.x/10; 
        auto z = movement.y/10;
        player.position.x += x;
        player.position.z += z;

        player.camera.position.x += x;
        player.camera.position.z += z;
        player.camera.target = player.position;

        player.angle = angle.x/angle.y;
    }

    if (IsKeyDown(KEY_N) && !std::ranges::contains(pressed_keys, KEY_N)) {
        player_stats.mana -= 1;
        pressed_keys.push_back(KEY_N);
    } else if (IsKeyDown(KEY_M) && !std::ranges::contains(pressed_keys, KEY_M)) {
        player_stats.mana += 1;
        pressed_keys.push_back(KEY_M);
    }

    mouse_pos = mouse_xz_in_world(GetMouseRay(GetMousePosition(), player.camera));
    float distance_to_player = std::abs(Vector2Distance((Vector2){player.position.x, player.position.z}, (Vector2){mouse_pos.x, mouse_pos.y}));
    player.mouse_in_reach = distance_to_player <= 100.0f;
}

int main() {
    InitWindow(0, 0, "Aetas Magus");
    SetWindowState(FLAG_FULLSCREEN_MODE);
    SetExitKey(KEY_NULL);

    Vector2 screen = (Vector2){ (float)GetScreenWidth(), (float)GetScreenHeight() };

    Player player = {
        .position = { 0.0f, 10.0f, 0.0f },
        .model = LoadModel("./assets/player/player.obj"),
        .camera = {0},
    };
    auto spell_idx = player.add_spell(Spell(Spell::Frost_Nova, Rarity::Rare, 5));
    PlayerStats player_stats(100, 100, 2);
    player_stats.mana = 10;
    player_stats.equip_spell(player.spellbook, spell_idx, 0);

    player.camera.position = (Vector3){ 30.0f, 70.0f, 0.0f };
    player.camera.target = player.position;
    player.camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    player.camera.fovy = 90.0f;
    player.camera.projection = CAMERA_PERSPECTIVE;

    Vector2 mouse_pos = (Vector2){0.0f, 0.0f};

    Textures textures;

    Shader fxaa_shader = LoadShader(0, "./assets/fxaa.glsl");
    int resolutionLoc = GetShaderLocation(fxaa_shader, "resolution");
    SetShaderValue(fxaa_shader, resolutionLoc, &screen, SHADER_UNIFORM_VEC2);

    RenderTexture2D target = LoadRenderTexture(screen.x, screen.y);

    bool tick = true;
    double mili_accum = 0;

    while (!WindowShouldClose()) {
        double time = GetTime();
        if (tick) {
            tick = false;
            PollInputEvents();
            update(player, player_stats, mouse_pos, screen, fxaa_shader, resolutionLoc, target);
        }

        BeginTextureMode(target);
            ClearBackground(WHITE);

            BeginMode3D(player.camera);
                DrawGrid(1000, 5.0f);
                DrawModelEx(player.model, player.position, (Vector3){ 0.0f, 1.0f, 0.0f }, player.angle, (Vector3){ 0.2f, 0.2f, 0.2f }, ORANGE);

                DrawCylinder((Vector3){ mouse_pos.x, 0.0f, mouse_pos.y },
                             5.0f, 5.0f, 1.0f, 128,
                             player.mouse_in_reach ? (Color){ 235, 216, 91, 127 } : (Color){ 237, 175, 164, 127 });
            EndMode3D();

            draw_ui(textures, player_stats, screen);
        EndTextureMode();

        int textureLoc = GetShaderLocation(fxaa_shader, "texture0");
        SetShaderValueTexture(fxaa_shader, textureLoc, target.texture); 

        BeginDrawing();
            ClearBackground(WHITE);

            BeginShaderMode(fxaa_shader);
                DrawTextureRec(target.texture,
                               (Rectangle){ 0.0f, 0.0f, (float)target.texture.width, -(float)target.texture.height },
                               (Vector2){ 0.0f, 0.0f },
                               WHITE);
            EndShaderMode();
        EndDrawing();
        SwapScreenBuffer();

        mili_accum += GetTime() - time;
        if (mili_accum > TICKS/1000) {
            mili_accum = 0;
            tick = true;
        }
    }

    UnloadShader(fxaa_shader);
    UnloadRenderTexture(target);
    UnloadModel(player.model);

    CloseWindow();

    return 0;
}
