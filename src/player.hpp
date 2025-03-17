#pragma once

#include "assets.hpp"
#include "enemies_spawner.hpp"
#include "hitbox.hpp"
#include "raylib.h"
#include "spell.hpp"
#include <cstdint>

using Player = struct Player {
    static constexpr float visibility_radius = 450.0f;
    // for some reason .x moves on the y axis and .y moves on the x axis.... fuck me
    static constexpr Vector2 visibility_center_offset = {-280.0f, 0.0f};

    Vector3 prev_position;
    Vector3 position;
    Vector3 interpolated_position;
    float angle = 0.0f;

    ModelAnimation* animations;
    int animationsCount;
    int animationCurrent = 0;
    int animationIndex = 0;

    shapes::Circle hitbox;
    Camera3D camera = {0};

    static const Vector3 camera_offset;
    static const float model_scale;

    uint32_t max_health = 100;
    uint32_t health = max_health;
    // per second
    uint32_t health_regen = 1;
    uint32_t max_mana = 100;
    uint32_t mana = max_mana;
    // per second
    uint32_t mana_regen = 10;
    uint16_t lvl = 0;
    // exp collected since level up
    uint32_t exp = 0;
    uint32_t exp_to_next_lvl = 100;
    // 10 max spells
    uint8_t max_spells = 2;
    // uint32_t::max means no spell is equipped
    // otherwise index of spell in spellbook
    std::vector<uint32_t> equipped_spells;
    uint8_t tick_counter = 0;

    Player(Vector3 position, assets::Store& assets);

    Player(Player&) = delete;
    Player& operator=(Player&) = delete;

    Player(Player&&) noexcept = default;
    Player& operator=(Player&&) noexcept = default;

    void update_interpolated_pos(double mili_accum);
    void draw_model(assets::Store& assets) const;

    // returns how much exp is required to level up from `lvl - 1` to `lvl`
    static uint32_t exp_to_lvl(uint16_t lvl);
    // returns if the player leveled up
    bool add_exp(uint32_t e);
    std::optional<std::reference_wrapper<const Spell>> get_equipped_spell(int idx, const SpellBook& spellbook) const;

    // -2 - slot out of range
    // -1 - spell isn't inside the spellbook
    // 0 - ok
    int equip_spell(uint32_t spellbook_idx, uint8_t slot_id, const SpellBook& spellbook);
    void tick(Vector2 movement, float angle, SpellBook& spellbook);
    void cast_equipped(int idx, const Vector2& player_position, const Vector2& mouse_pos, SpellBook& spellbook,
                       const Enemies& enemies);

    ~Player();
};

using PlayerStats = struct PlayerStats {
    SpellBook spellbook = {};

    uint32_t add_spell_to_spellbook(Spell&& spell);
};
