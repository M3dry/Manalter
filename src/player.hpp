#pragma once

#include "assets.hpp"
#include "enemies_spawner.hpp"
#include "hitbox.hpp"
#include "raylib.h"
#include "spell.hpp"
#include "stats.hpp"
#include "power_up.hpp"
#include <cstdint>
#include <memory>

struct Player {
    static constexpr float visibility_radius = 450.0f;
    // for some reason .x moves on the y axis and .y moves on the x axis.... fuck me
    static constexpr Vector2 visibility_center_offset = {-280.0f, 0.0f};
    static constexpr uint8_t max_spell_count = 10;

    Vector3 prev_position;
    Vector3 position;
    Vector3 interpolated_position;
    float angle = 0.0f;

    ModelAnimation* animations;
    int animationsCount;
    int animationCurrent = 0;
    int animationIndex = 0;

    shapes::Circle hitbox;
    Camera3D camera{};

    static const Vector3 camera_offset;
    static const float model_scale;

    std::vector<PowerUp> power_ups;
    PlayerStats stats;
    uint32_t health = stats.max_health.get();
    uint64_t mana = stats.max_mana.get();
    uint16_t lvl = 0;
    // exp collected since level up
    uint32_t exp = 0;
    uint32_t exp_to_next_lvl = 100;
    // 10 max spells
    uint8_t unlocked_spell_count = 1;
    // std::size_t::max means no spell is equipped
    // otherwise index of spell in spellbook
    std::unique_ptr<uint64_t[]> equipped_spells;
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
    std::optional<std::reference_wrapper<const Spell>> get_equipped_spell(uint8_t idx, const SpellBook& spellbook) const;

    // -2 - slot out of range
    // -1 - spell isn't inside the spellbook
    // 0 - ok
    int equip_spell(uint64_t spellbook_idx, uint8_t slot_id, const SpellBook& spellbook);
    void tick(Vector2 movement, float angle, SpellBook& spellbook);
    void cast_equipped(uint8_t idx, const Vector2& player_position, const Vector2& mouse_pos, SpellBook& spellbook,
                       const Enemies& enemies);

    void add_power_up(PowerUp&& powerup);

    static bool unlocks_spell_slot(uint16_t lvl);

    ~Player();
};

struct PlayerSave {
    SpellBook spellbook = {};
    uint64_t souls = 0;

    uint64_t add_spell_to_spellbook(Spell&& spell);
};
