#pragma once

#include "assets.hpp"
#include "enemies_spawner.hpp"
#include "hitbox.hpp"
#include "power_up.hpp"
#include "raylib.h"
#include "spell.hpp"
#include "stats.hpp"
#include <cstdint>
#include <filesystem>
#include <memory>

struct Player {
    static constexpr float visibility_radius = 450.0f;
    static constexpr Vector2 visibility_center_offset = {0.0f, -280.0f};
    static constexpr uint8_t max_spell_count = 10;

    Vector3 prev_position;
    Vector2 prev_total_position;
    Vector3 position;
    Vector2 total_position;
    Vector3 interpolated_position;
    Vector2 interpolated_total_position;
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
    uint64_t get_equipped_spell(uint8_t idx) const;

    // -2 - slot out of range
    // -1 - spell isn't inside the spellbook
    // 0 - ok
    int equip_spell(uint64_t spellbook_idx, uint8_t slot_id, const SpellBook& spellbook);
    void tick(Vector2 movement, float angle);
    uint64_t can_cast(uint8_t idx, const SpellBook& spellbook);

    void add_power_up(PowerUp&& powerup);

    static bool unlocks_spell_slot(uint16_t lvl);

    ~Player();
};

class PlayerSave {
  public:
    inline static const std::filesystem::path save_path = std::filesystem::path("./") / "save.bin";

    void load_save();
    void save();

    inline const SpellBook& get_spellbook() const {
        return spellbook;
    }
    inline const SpellBook& get_stash() const {
        return stash_book;
    }
    inline uint64_t get_souls() const {
        return souls;
    }

    void create_default_spell();
    void remove_default_spell();
    uint64_t add_spell_to_spellbook(Spell&& spell);
    void remove_spell(uint64_t ix);
    void cast_spell(uint64_t spell_id, const Vector2& player_position, const Vector2& mouse_pos, Enemies& enemies, uint64_t& mana);
    void tick_spellbook();

    inline void add_souls(uint64_t s) {
        souls += s;
    }

    void serialize(std::ostream& out);
    static PlayerSave deserialize(std::istream& in, version version);
  private:
    SpellBook spellbook;
    SpellBook stash_book;
    uint64_t souls = 0;
    std::vector<uint64_t> spells_to_tick;
    bool default_spell = false;
};
