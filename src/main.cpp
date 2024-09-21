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

#define TICKS 20

bool is_key_pressed(const std::vector<std::pair<int, bool>>& pressed_keys, bool plus_handled, int key) {
    for (const auto& [k, handled] : pressed_keys) {
        if (!plus_handled && handled) return false;
        if (key == k) return true;
    }

    return false;
}

// very generic names, TODO: maybe come up with better ones
enum struct Rarity {
    Common = 0,
    Uncommon,
    Rare,
    Epic,
    Legendary,
    Size
};

static const std::string rarity_frame_path = "./assets/spell-icons/frames";
static const std::array<std::string, static_cast<std::size_t>(Rarity::Size)> rarity_frames = {
    "common.png", // Common
    "uncommon.png", // Uncommon
    "rare.png", // Rare
    "epic.png", // Epic
    "legendary.png", // Legendary
};

using Spell = struct Spell {
    enum Name {
        Fire_Wall = 0,
        Falling_Icicle,
        Lightning_Strike,
        Frost_Nova,
        Void_Implosion,
        Mana_Detonation,
        NameSize, // keep last
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

    using Types = std::unordered_set<Type>;

    static const std::array<Types, NameSize> type_map;
    // in ticks, doesn't change with rarity
    static const std::array<int, NameSize> cooldown_map;
    static const std::string icon_path;
    // just file name, path is `Spell::icon_path`
    static const std::array<std::string, NameSize> icon_map;
    static const std::array<float, static_cast<std::size_t>(Rarity::Size)> rarity_multiplier;

    Name name;
    Rarity rarity;
    uint16_t lvl;
    // exp collected since level up
    uint32_t exp;
    // in ticks
    int curr_cooldown;

    Spell(Name name, Rarity rarity, uint16_t lvl)
        : name(name), rarity(rarity), lvl(lvl), exp(0), curr_cooldown(0) {};

    const Types& get_type() const {
        return Spell::type_map[name];
    };

    int get_cooldown() const {
        return Spell::cooldown_map[name];
    }

    const std::string& get_icon() const {
        return Spell::icon_map[name];
    }

    float get_rarity_multiplier() const {
        return Spell::rarity_multiplier[static_cast<std::size_t>(rarity)];
    }
};

const std::array<Spell::Types, Spell::NameSize> Spell::type_map = {
    std::unordered_set({ Spell::AOE, Spell::AtMouse, Spell::Fire }), // Fire Wall
    std::unordered_set({ Spell::AtMouse, Spell::Ice }), // Falling Icicle
    std::unordered_set({ Spell::AtMouse, Spell::Lightning }), // Lightning Strike
    std::unordered_set({ Spell::AOE,  Spell::AtMouse, Spell::Ice }), // Frost Nova
    std::unordered_set({ Spell::AOE, Spell::AtMouse, Spell::DarkMagic }), // Void Implosion
    std::unordered_set({ Spell::AOE, Spell::AtPlayer, Spell::DarkMagic }), // Mana Detonation
};

const std::array<int, Spell::NameSize> Spell::cooldown_map = {
    10, // Fire Wall
    10, // Falling Icicle
    15, // Lightning Strike
    20, // Frost Nova
    40, // Void Implosion
    40, // Mana Detonation
};

const std::string Spell::icon_path = "./assets/spell-icons";
const std::array<std::string, Spell::NameSize> Spell::icon_map = {
    "fire-wall.png", // Fire Wall
    "falling-icicle.png", // Falling Icicle
    "lightning-strike.png", // Lightning Strike
    "frost-nova.png", // Frost Nova
    "void-implosion.png", // Void Implosion
    "mana-detonation.png", // Mana Detonation
};

const std::array<float, static_cast<std::size_t>(Rarity::Size)> Spell::rarity_multiplier = {
    1.0f, // Common
    1.1f, // Uncommon
    1.3f, // Rare
    1.6f, // Epic
    2.0f, // Legendary
};

using SpellBook = std::vector<Spell>;

using Player = struct Player {
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

class Textures {
  public:
    enum GeneralId {
        EmptySpellSlot,
        LockedSlot,
    };

    enum RenderId {
        Target,
        CircleUI,
        SpellBarUI,
    };

    Textures(Vector2 screen) : general_map({}), spell_map({}), render_map({}), spell_frame_map({}) {
        add_general_textures();
        add_spell_textures();
        add_render_textures(screen);
        add_spell_frames();
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

    RenderTexture2D operator [](RenderId id) {
        return render_map[id];
    }

    Texture2D operator [](Rarity id) {
        return spell_frame_map[id];
    }

    void update_target_size(Vector2 screen) {
        UnloadRenderTexture(render_map[Textures::Target]);
        render_map[Textures::Target] = LoadRenderTexture(screen.x, screen.y);
    }

  private:
    std::unordered_map<GeneralId, Texture2D> general_map;
    std::unordered_map<Spell::Name, Texture2D> spell_map;
    std::unordered_map<RenderId, RenderTexture2D> render_map;
    std::unordered_map<Rarity, Texture2D> spell_frame_map;

    void add_general_textures() {
        Image img = LoadImage("./assets/spell-icons/empty-slot.png");
        general_map[EmptySpellSlot] = LoadTextureFromImage(img);
        SetTextureFilter(general_map[EmptySpellSlot], TEXTURE_FILTER_TRILINEAR);
        UnloadImage(img);

        Image img2 = LoadImage("./assets/spell-icons/locked-slot.png");
        general_map[LockedSlot] = LoadTextureFromImage(img2);
        SetTextureFilter(general_map[LockedSlot], TEXTURE_FILTER_TRILINEAR);
        UnloadImage(img2);
    }

    void add_spell_textures() {
        for (int name_i = 0; name_i < Spell::NameSize; name_i++) {
            auto name = static_cast<Spell::Name>(name_i);
            Image img = LoadImage((Spell::icon_path + "/" + Spell::icon_map[name_i]).c_str());
            spell_map[name] = LoadTextureFromImage(img);
            SetTextureFilter(spell_map[name], TEXTURE_FILTER_TRILINEAR);
            UnloadImage(img);
        }
    };

    void add_render_textures(Vector2 screen) {
        render_map[Textures::Target] = LoadRenderTexture(screen.x, screen.y);
        render_map[CircleUI] = LoadRenderTexture(1023, 1023); // odd to have a center pixel
        render_map[SpellBarUI] = LoadRenderTexture(512, 128);
    }

    void add_spell_frames() {
        for (int rarity_i = 0; rarity_i < static_cast<int>(Rarity::Size); rarity_i++) {
            auto rarity = static_cast<Rarity>(rarity_i);
            Image img = LoadImage((rarity_frame_path + "/" + rarity_frames[rarity_i]).c_str());
            spell_frame_map[rarity] = LoadTextureFromImage(img);
            SetTextureFilter(spell_frame_map[rarity], TEXTURE_FILTER_TRILINEAR);
            UnloadImage(img);
        }
    }
};

Vector2 mouse_xz_in_world(Ray mouse) {
    const float x = -mouse.position.y/mouse.direction.y;

    assert(mouse.position.y + x*mouse.direction.y == 0);

    return (Vector2){ mouse.position.x + x*mouse.direction.x, mouse.position.z + x*mouse.direction.z };
}

void draw_circle_texture(Texture2D texture, Vector2 center, float radius, Color color) {
    DrawTextureEx(texture, (Vector2){ center.x - radius, center.y - radius }, 0.0f, 0.001f * radius, color);
}

// Will draw ui to the corresponding RenderTextures
void draw_ui(Textures& textures, const PlayerStats& player_stats, const Vector2& screen) {
    static const uint32_t padding = 10;
    static const uint32_t outer_radius = 1023/2;
    static const Vector2 center = (Vector2){ (float)outer_radius, (float)outer_radius };

    BeginTextureMode(textures[Textures::CircleUI]);
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

    // Spell Bar
    BeginTextureMode(textures[Textures::SpellBarUI]);
        ClearBackground(BLANK);

        DrawRectangle(0, 0, 512, 128, RED);

        const int spell_dim = 128/2; // 2x5 spells

        for (int i = 0; i < 10; i++) {
            int row = i/5;
            int col = i - 5*row;

            if (i >= player_stats.max_spells) {
                DrawTexturePro(textures[Textures::LockedSlot],
                               (Rectangle){ 1.0f, 0.0f, (float)textures[Textures::LockedSlot].width, (float)textures[Textures::LockedSlot].height },
                               (Rectangle){ (float)col*spell_dim, (float)spell_dim*row, (float)spell_dim, (float)spell_dim },
                               Vector2Zero(), 0.0f, WHITE);
                continue;
            }

            if (auto spell_opt = player_stats.get_equipped_spell(i); spell_opt) {
                const Spell& spell = *spell_opt;

                // TODO: rn ignoring the fact that the frame draws over the spell icon
                Texture2D spell_tex = textures[spell.name];
                DrawTexturePro(spell_tex,
                               (Rectangle){ 0.0f, 0.0f, (float)spell_tex.width, (float)spell_tex.height },
                               (Rectangle){ (float)col*spell_dim, (float)spell_dim*row, (float)spell_dim, (float)spell_dim },
                               Vector2Zero(), 0.0f, WHITE);

                BeginBlendMode(BLEND_ADDITIVE);
                    float cooldown_height = spell_dim * spell.curr_cooldown/spell.get_cooldown();
                    DrawRectangle(col*spell_dim, spell_dim*row + (spell_dim - cooldown_height), spell_dim, cooldown_height, { 130, 130, 130, 128 });
                EndBlendMode();

                Texture2D frame = textures[spell.rarity];
                DrawTexturePro(frame,
                               (Rectangle){ 0.0f, 0.0f, (float)frame.width, (float)frame.height },
                               (Rectangle){ (float)col*spell_dim, (float)spell_dim*row, (float)spell_dim, (float)spell_dim },
                               Vector2Zero(), 0.0f, WHITE);
            } else {
                DrawTexturePro(textures[Textures::EmptySpellSlot],
                               (Rectangle){ 0.0f, 0.0f, (float)textures[Textures::EmptySpellSlot].width, (float)textures[Textures::EmptySpellSlot].height },
                               (Rectangle){ (float)col*spell_dim, (float)row*spell_dim, (float)spell_dim, (float)spell_dim },
                               Vector2Zero(), 0.0f, WHITE);
            }
        }

        // RADAR/MAP???
        DrawRectangle(spell_dim*5, 0, 512 - spell_dim*5, 128, GRAY);
    EndTextureMode();
}

void update(const std::vector<std::pair<int, bool>>& pressed_keys, Textures& textures, Player& player, PlayerStats& player_stats, Vector2& mouse_pos, Vector2& screen, Shader fxaa_shader, int resolutionLoc) {
    static const int keys[] = { KEY_A, KEY_S, KEY_D, KEY_W };
    static const Vector2 movements[] = { { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 } };
    static const float angles[] = { 0.0f, 90.0f, 180.0f, 270.0f };

    Vector2 movement = { 0, 0 };
    Vector2 angle = { 0.0f, 0.0f };

    if (IsWindowResized()) {
        screen = (Vector2){ (float)GetScreenWidth(), (float)GetScreenHeight() };
        SetShaderValue(fxaa_shader, resolutionLoc, &screen, SHADER_UNIFORM_VEC2);

        textures.update_target_size(screen);
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

    if (is_key_pressed(pressed_keys, false, KEY_N)) {
        player_stats.mana -= 1;
    }
    if (is_key_pressed(pressed_keys, false, KEY_M)) {
        player_stats.mana += 1;
    }
    if (is_key_pressed(pressed_keys, false, KEY_FOUR)) {
        player_stats.cast_equipped(3);
    }

    mouse_pos = mouse_xz_in_world(GetMouseRay(GetMousePosition(), player.camera));
    float distance_to_player = std::abs(Vector2Distance((Vector2){player.position.x, player.position.z}, (Vector2){mouse_pos.x, mouse_pos.y}));
    player.mouse_in_reach = distance_to_player <= 100.0f;

    player_stats.tick_cooldown();
}

int main() {
    InitWindow(0, 0, "Aetas Magus");
    SetWindowState(FLAG_FULLSCREEN_MODE);
    SetExitKey(KEY_NULL);
    // DisableCursor();

    Vector2 screen = (Vector2){ (float)GetScreenWidth(), (float)GetScreenHeight() };

    Player player = {
        .position = { 0.0f, 10.0f, 0.0f },
        .model = LoadModel("./assets/player/player.obj"),
        .camera = {0},
    };
    PlayerStats player_stats(100, 100, 4);
    player_stats.mana = 10;
    player_stats.health = 30;
    player_stats.add_exp(50);
    auto frost_nove_idx = player_stats.add_spell_to_spellbook(Spell(Spell::Frost_Nova, Rarity::Rare, 5));
    Spell void_implosion_spell = Spell(Spell::Void_Implosion, Rarity::Epic, 20);
    void_implosion_spell.curr_cooldown = 40;
    auto void_implosion = player_stats.add_spell_to_spellbook(void_implosion_spell);
    std::println("SPELLBOOK_SIZE: {}", player_stats.spellbook.size());
    std::println("EQUIP_SPELL: {}", player_stats.equip_spell(frost_nove_idx, 0));
    std::println("EQUIP_SPELL: {}", player_stats.equip_spell(void_implosion, 3));

    player.camera.position = (Vector3){ 30.0f, 70.0f, 0.0f };
    player.camera.target = player.position;
    player.camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    player.camera.fovy = 90.0f;
    player.camera.projection = CAMERA_PERSPECTIVE;

    Vector2 mouse_pos = (Vector2){0.0f, 0.0f};

    Textures textures(screen);

    Shader fxaa_shader = LoadShader(0, "./assets/fxaa.glsl");
    int resolutionLoc = GetShaderLocation(fxaa_shader, "resolution");
    SetShaderValue(fxaa_shader, resolutionLoc, &screen, SHADER_UNIFORM_VEC2);

    double mili_accum = 0;

    std::vector<int> registered_keys = { KEY_N, KEY_M, KEY_FOUR };
    std::vector<std::pair<int, bool>> pressed_keys; // (KEY, if it was handled)
    int key = 0;

    while (!WindowShouldClose()) {
        double time = GetTime();

        BeginTextureMode(textures[Textures::Target]);
            ClearBackground(WHITE);

            BeginMode3D(player.camera);
                DrawGrid(1000, 5.0f);
                DrawModelEx(player.model, player.position, (Vector3){ 0.0f, 1.0f, 0.0f }, player.angle, (Vector3){ 0.2f, 0.2f, 0.2f }, ORANGE);

                DrawCylinder((Vector3){ mouse_pos.x, 0.0f, mouse_pos.y },
                             5.0f, 5.0f, 1.0f, 128,
                             player.mouse_in_reach ? (Color){ 235, 216, 91, 127 } : (Color){ 237, 175, 164, 127 });
            EndMode3D();
        EndTextureMode();

        int textureLoc = GetShaderLocation(fxaa_shader, "texture0");
        SetShaderValueTexture(fxaa_shader, textureLoc, textures[Textures::Target].texture); 

        draw_ui(textures, player_stats, screen);

        BeginDrawing();
            ClearBackground(WHITE);

            BeginShaderMode(fxaa_shader);
                DrawTextureRec(textures[Textures::Target].texture,
                               (Rectangle){ 0.0f, 0.0f, (float)textures[Textures::Target].texture.width, -(float)textures[Textures::Target].texture.height },
                               Vector2Zero(), WHITE);
            EndShaderMode();

            float circle_ui_dim = screen.x * 1/8;
            static const float padding = 10;
            DrawTexturePro(textures[Textures::CircleUI].texture,
                           (Rectangle){ 0.0f, 0.0f, (float)textures[Textures::CircleUI].texture.width, -(float)textures[Textures::CircleUI].texture.height },
                           (Rectangle){ screen.x - circle_ui_dim - padding, screen.y - circle_ui_dim - padding, circle_ui_dim, circle_ui_dim },
                           Vector2Zero(), 0.0f, WHITE);

            float spell_bar_width = screen.x/4.0f;
            float spell_bar_height = spell_bar_width/4.0f;
            DrawTexturePro(textures[Textures::SpellBarUI].texture,
                           (Rectangle){ 0.0f, 0.0f, (float)textures[Textures::SpellBarUI].texture.width, -(float)textures[Textures::SpellBarUI].texture.height },
                           (Rectangle){ (screen.x - spell_bar_width)/2.0f, screen.y - spell_bar_height, spell_bar_width, spell_bar_height },
                           Vector2Zero(), 0.0f, WHITE);
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
        if (mili_accum*1000 > (1000/TICKS)) {
            mili_accum = 0;

            update(pressed_keys, textures, player, player_stats, mouse_pos, screen, fxaa_shader, resolutionLoc);
            for (auto& [_, handled] : pressed_keys) {
                handled = true;
            }
        }
    }

    UnloadShader(fxaa_shader);
    UnloadModel(player.model);

    CloseWindow();

    return 0;
}
