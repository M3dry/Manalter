#include "player.hpp"
#include "seria_deser.hpp"
#include "spell.hpp"
#include "spell_caster.hpp"
#include "utility.hpp"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>

const Vector3 Player::camera_offset = (Vector3){60.0f, 140.0f, 0.0f};
const float Player::model_scale = 0.2f;

Player::Player(Vector3 position, assets::Store& assets)
    : prev_position(position), position(position), interpolated_position(position), animations(nullptr),
      hitbox((Vector2){position.x, position.z}, 8.0f) {
    equipped_spells.reset(new uint64_t[Player::max_spell_count]);
    for (std::size_t i = 0; i < Player::max_spell_count; i++) {
        equipped_spells[i] = std::numeric_limits<uint64_t>::max();
    }

    camera.position = camera_offset;
    camera.target = position;
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 90.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    auto model = assets[assets::Player];

    animations = LoadModelAnimations("./assets/player/player.glb", &animationsCount);
    UpdateModelAnimation(model, animations[animationIndex], animationCurrent);
}

void Player::update_interpolated_pos(double accum_time) {
    interpolated_position = Vector3Lerp(prev_position, position, static_cast<float>(accum_time) * TICKS);
    prev_position = interpolated_position;
    camera.target = interpolated_position;
    camera.position = Vector3Lerp(camera.position, Vector3Add(camera_offset, interpolated_position), 1.0f);
}

void Player::draw_model(assets::Store& assets) const {
    UpdateModelAnimation(assets[assets::Player], animations[animationIndex], animationCurrent);
    DrawModelEx(assets[assets::Player], interpolated_position, (Vector3){0.0f, 1.0f, 0.0f}, angle,
                (Vector3){model_scale, model_scale, model_scale}, WHITE);

#ifdef DEBUG
    DrawSphere((Vector3){hitbox.center.x, 1.0f, hitbox.center.y}, 1.0f, BLUE);
    hitbox.draw_3D(RED, 1.0f, Vector2Zero());
#endif
}

uint32_t Player::exp_to_lvl(uint16_t lvl) {
    return 100 * static_cast<uint32_t>(std::pow(lvl, 2));
}

bool Player::add_exp(uint32_t e) {
    exp += e;
    if (exp >= exp_to_next_lvl) {
        lvl++;
        exp -= exp_to_next_lvl;
        exp_to_next_lvl = Player::exp_to_lvl(lvl + 1);

        if (unlocks_spell_slot(lvl)) unlocked_spell_count++;
        return true;
    }

    return false;
}

uint64_t Player::get_equipped_spell(uint8_t idx) const {
    if (idx >= unlocked_spell_count || equipped_spells[idx] == std::numeric_limits<uint64_t>::max())
        return std::numeric_limits<uint64_t>::max();

    return equipped_spells[idx];
}

int Player::equip_spell(uint64_t spellbook_idx, uint8_t slot_id, const SpellBook& spellbook) {
    if (spellbook.size() <= spellbook_idx) return -1;
    if (unlocked_spell_count <= slot_id) return -2;

    equipped_spells[slot_id] = spellbook_idx;

    return 0;
}

void Player::tick(Vector2 movement, float new_angle) {
    int lastIndex = animationIndex;
    if (movement.x == 0 && movement.y == 0) {
        animationIndex = 0;
    } else {
        prev_position = position;
        position.x += movement.x * stats.speed.get();
        position.z += movement.y * stats.speed.get();
        angle = new_angle;

        arena::loop_around(position.x, position.z);

        hitbox.center = xz_component(position);

        animationIndex = 2;
    }

    if (lastIndex != animationIndex) {
        animationCurrent = 0;
    } else {
        animationCurrent = (animationCurrent + 3) % animations[animationIndex].frameCount;
    }

    if (tick_counter == TICKS) {
        tick_counter = 0;
        health += stats.health_regen.get();
        mana += stats.mana_regen.get();

        if (health > stats.max_health.get()) health = stats.max_health.get();
        if (mana > stats.max_mana.get()) mana = stats.max_mana.get();
    }

    tick_counter++;
}

uint64_t Player::can_cast(uint8_t idx, const SpellBook& spellbook) {
    if (idx >= 10 || idx >= unlocked_spell_count || equipped_spells[idx] == std::numeric_limits<uint64_t>::max())
        return std::numeric_limits<uint64_t>::max();

    auto spell_id = equipped_spells[idx];
    const auto& spell = spellbook[spell_id];
    if (mana < spell.stats.manacost.get() || spell.current_cooldown > 0) return std::numeric_limits<uint64_t>::max();

    return spell_id;
}

void Player::add_power_up(PowerUp&& powerup) {
    powerup.apply(stats);
    power_ups.emplace_back(std::forward<PowerUp>(powerup));
}

bool Player::unlocks_spell_slot(uint16_t lvl) {
    switch (lvl) {
        case 1:
        case 2:
        case 5:
        case 7:
        case 10:
        case 13:
        case 15:
        case 17:
        case 20:
            return true;
    }

    return false;
}

Player::~Player() {
    UnloadModelAnimations(animations, animationsCount);
}

void PlayerSave::load_save() {
    namespace fs = std::filesystem;

    if (!fs::exists(save_path.parent_path())) {
        fs::create_directories(save_path.parent_path());
    }

    if (!fs::exists(save_path)) {
        save();
        return;
    }

    std::ifstream input(save_path, std::ios::binary);
    if (!input) {
        TraceLog(LOG_ERROR, "Couldn't open file for loading");
        return;
    }

    version save_version = seria_deser::deserialize_version(input);
    *this = PlayerSave::deserialize(input, save_version);
}

void PlayerSave::save() {
    std::ofstream output(save_path, std::ios::binary);
    if (!output) {
        TraceLog(LOG_ERROR, "Couldn't open file for saving");
        return;
    }

    seria_deser::serialize(CURRENT_VERSION, output);
    serialize(output);
}

void PlayerSave::create_default_spell() {
    if (default_spell) return;

    spellbook.emplace_front(spells::FrostNova{}, Rarity::Common, 1);
    default_spell = true;
}

void PlayerSave::remove_default_spell() {
    if (!default_spell) return;

    spellbook.pop_front();
    default_spell = false;
}

uint64_t PlayerSave::add_spell_to_spellbook(Spell&& spell) {
    spellbook.emplace_back(std::move(spell));

    return spellbook.size() - 1;
}

void PlayerSave::cast_spell(uint64_t spell_id, const Vector2& player_position, const Vector2& mouse_pos,
                            Enemies& enemies, uint64_t& mana) {
    if (spell_id == std::numeric_limits<uint64_t>::max()) return;

    auto& spell = spellbook[spell_id];

    if (caster::cast(spell_id, spell, player_position, mouse_pos, enemies)) {
        mana -= spell.stats.manacost.get();
        spell.current_cooldown = spell.cooldown;
        spells_to_tick.emplace_back(spell_id);
    }
}

void PlayerSave::tick_spellbook() {
    std::size_t i = 0;
    while (i < spells_to_tick.size()) {
        auto& spell = spellbook[spells_to_tick[i]];
        if (spell.current_cooldown == 0) {
            spells_to_tick[i] = spells_to_tick.back();
            spells_to_tick.pop_back();
            continue;
        }

        spell.current_cooldown--;
        i++;
    }
}

void PlayerSave::serialize(std::ostream& out) {
    seria_deser::serialize(souls, out);

    if (default_spell) {
        auto spell = std::move(spellbook.front());

        spellbook.pop_front();
        seria_deser::serialize(spellbook, out);
        spellbook.emplace_front(std::move(spell));
    } else {
        seria_deser::serialize(spellbook, out);
    }

    seria_deser::serialize(stash_book, out);
}

struct PlayerSaveV1 {
    uint64_t souls;
    SpellBook spellbook;

    static PlayerSaveV1 deserialize(std::istream& in, version version) {
        assert(version == 1);

        return PlayerSaveV1{
            .souls = seria_deser::deserialize<uint64_t>(in, version),
            .spellbook = seria_deser::deserialize<SpellBook>(in, version),
        };
    }
};

PlayerSave PlayerSave::deserialize(std::istream& in, version version) {
    PlayerSave ps;

    switch (version) {
        case 1: {
            auto ps1 = PlayerSaveV1::deserialize(in, version);

            ps.souls = ps1.souls;
            ps.spellbook = std::move(ps1.spellbook);
            return ps;
        }
        case 2: {
            ps.souls = seria_deser::deserialize<uint64_t>(in, version);
            ps.spellbook = seria_deser::deserialize<SpellBook>(in, version);
            ps.stash_book = seria_deser::deserialize<SpellBook>(in, version);

            return ps;
        }
        default:
            assert(false && "Can't handle this version");
    }
}
