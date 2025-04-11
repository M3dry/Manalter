#include "player.hpp"
#include "spell_caster.hpp"
#include "utility.hpp"
#include <cstdint>
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

std::optional<std::reference_wrapper<const Spell>> Player::get_equipped_spell(uint8_t idx,
                                                                              const SpellBook& spellbook) const {
    if (idx >= unlocked_spell_count || equipped_spells[idx] == std::numeric_limits<uint64_t>::max()) return {};

    return spellbook[equipped_spells[idx]];
}

int Player::equip_spell(uint64_t spellbook_idx, uint8_t slot_id, const SpellBook& spellbook) {
    if (spellbook.size() <= spellbook_idx) return -1;
    if (unlocked_spell_count <= slot_id) return -2;

    equipped_spells[slot_id] = spellbook_idx;

    return 0;
}

void Player::tick(Vector2 movement, float new_angle, SpellBook& spellbook) {
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

    for (std::size_t i = 0; i < 10 && i < unlocked_spell_count; i++) {
        if (equipped_spells[i] == std::numeric_limits<uint64_t>::max()) continue;

        Spell& spell = spellbook[equipped_spells[i]];

        if (spell.current_cooldown == 0) continue;
        spell.current_cooldown--;
    }

    tick_counter++;
}

void Player::cast_equipped(uint8_t idx, const Vector2& player_position, const Vector2& mouse_pos, SpellBook& spellbook,
                           const Enemies& enemies) {
    if (idx >= 10 || idx >= unlocked_spell_count || equipped_spells[idx] == std::numeric_limits<uint64_t>::max()) return;

    auto spell_id = equipped_spells[idx];
    Spell& spell = spellbook[spell_id];
    if (mana < spell.stats.manacost.get() || spell.current_cooldown > 0) return;

    if (caster::cast(spell_id, spell, player_position, mouse_pos, enemies)) {
        mana -= spell.stats.manacost.get();
        spell.current_cooldown = spell.cooldown;
    }
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

uint64_t PlayerSave::add_spell_to_spellbook(Spell&& spell) {
    spellbook.emplace_back(std::move(spell));

    return spellbook.size() - 1;
}
