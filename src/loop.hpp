#pragma once

#include "raylib.h"

#include <vector>
#include <utility>
#include <optional>
#include <cstdint>
#include <functional>
#include <variant>

#include "spell.hpp"
#include "assets.hpp"
#include "hitbox.hpp"

#define TICKS 20

using Player = struct Player {
    Vector3 prev_position;
    Vector3 position;
    Vector3 interpolated_position;
    float angle = 0.0f;
    Model model;
    // screw c++, this has to be here :(
    // can assume this is always a value
    std::optional<shapes::Polygon> hitbox;
    Camera3D camera = {0};
    bool mouse_in_reach = false;

    static const Vector3 camera_offset;
    static const float model_scale;

    Player(Vector3 position);
    void update_interpolated_pos(double mili_accum);
    void update_in_reach(Vector2 mouse);
    void update_position(Vector2 movement, float new_angle);
    void draw_model() const;
    ~Player();
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

    PlayerStats(uint32_t max_health, uint32_t max_mana, uint8_t max_spells);
    // Return how much exp is required to level up from `lvl - 1` to `lvl`
    static uint32_t exp_to_lvl(uint16_t lvl);
    void add_exp(uint32_t e);
    std::optional<std::reference_wrapper<const Spell>> get_equipped_spell(int idx) const;
    uint32_t add_spell_to_spellbook(Spell&& spell);
    // -2 - slot out of range
    // -1 - spell isn't inside the spellbook
    // 0 - ok
    int equip_spell(uint32_t spellbook_idx, uint8_t slot_id);
    void tick_cooldown();
    void cast_equipped(int idx);
};

class ItemDrop {
  public:
    enum struct Type {
        Spell,
        // TODO: Armor, jewelry
    };

    static const float hitbox_radius;

    Type type;
    shapes::Circle hitbox;

    ItemDrop(Vector2 center, Spell&& spell);
    void draw_name(std::function<Vector2(Vector3)> to_screen_coords) const;
    void move_item(std::function<void(Spell&&)> spell_f);
    const std::string& get_name() const;
  private:
    std::variant<Spell> item;
};

class Loop {
  public:
    Vector2 screen;
    Player player;
    PlayerStats player_stats;
    Vector2 mouse_pos;
    assets::Store assets;
    std::vector<ItemDrop> item_drops;
    std::vector<int> registered_keys;

    Loop(int width, int height);
    void operator()();
  private:
    double prev_time = 0.0f;
    double accum_time = 0.0f;
    std::vector<std::pair<int, bool>> pressed_keys; // (Key, if it was handled)

    void update();
};
