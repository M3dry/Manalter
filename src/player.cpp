#include "player.hpp"
#include "spell_caster.hpp"
#include "utility.hpp"

const Vector3 Player::camera_offset = (Vector3){60.0f, 140.0f, 0.0f};
const float Player::model_scale = 0.2f;

Player::Player(Vector3 position, assets::Store& assets)
    : prev_position(position), position(position), interpolated_position(position),
      animations(nullptr),
      hitbox((Vector2){position.x, position.z}, 8.0f), equipped_spells(10, UINT32_MAX) {
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
    interpolated_position = Vector3Lerp(prev_position, position, accum_time * TICKS);
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
    hitbox.draw_3D(RED, 1.0f);
#endif
}

uint32_t Player::exp_to_lvl(uint16_t lvl) {
    return 100 * std::pow(lvl, 2);
}

void Player::add_exp(uint32_t e) {
    exp += e;
    if (exp >= exp_to_next_lvl) {
        lvl++;
        exp -= exp_to_next_lvl;
        exp_to_next_lvl = Player::exp_to_lvl(lvl + 1);
    }
}

std::optional<std::reference_wrapper<const Spell>> Player::get_equipped_spell(int idx, const SpellBook& spellbook) const {
    if (idx >= 10 || idx >= max_spells || equipped_spells[idx] == UINT32_MAX) return {};

    return spellbook[equipped_spells[idx]];
}

int Player::equip_spell(uint32_t spellbook_idx, uint8_t slot_id, const SpellBook& spellbook) {
    if (spellbook.size() <= spellbook_idx) return -1;
    if (max_spells <= slot_id) return -2;

    equipped_spells[slot_id] = spellbook_idx;

    return 0;
}

void Player::tick(Vector2 movement, float angle, SpellBook& spellbook) {
    int lastIndex = animationIndex;
    if (movement.x == 0 && movement.y == 0) {
        animationIndex = 0;
    } else {
        prev_position = position;
        position.x += movement.x;
        position.z += movement.y;
        this->angle = angle;

        if (position.x > 500) position.x -= 1000;
        if (position.z > 500) position.z -= 1000;
        if (position.x < -500) position.x += 1000;
        if (position.z < -500) position.z += 1000;

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
        health += health_regen;
        mana += mana_regen;

        if (health > max_health) health = max_health;
        if (mana > max_mana) mana = max_mana;
    }

    for (int i = 0; i < 10 && i < max_spells; i++) {
        if (equipped_spells[i] == UINT32_MAX) continue;

        Spell& spell = spellbook[equipped_spells[i]];

        if (spell.current_cooldown == 0) continue;
        spell.current_cooldown--;
    }

    tick_counter++;
}

void Player::cast_equipped(int idx, const Vector2& player_position, const Vector2& mouse_pos, SpellBook& spellbook, const Enemies& enemies) {
    if (idx >= 10 || idx >= max_spells || equipped_spells[idx] == UINT32_MAX) return;

    auto spell_id = equipped_spells[idx];
    Spell& spell = spellbook[spell_id];
    if (mana < spell.manacost || spell.current_cooldown > 0) return;

    if (caster::cast(spell_id, spell, player_position, mouse_pos, enemies)) {
        mana -= spell.manacost;
        spell.current_cooldown = spell.cooldown;
    }
}

Player::~Player() {
    UnloadModelAnimations(animations, animationsCount);
}

uint32_t PlayerStats::add_spell_to_spellbook(Spell&& spell) {
    spellbook.emplace_back(std::move(spell));

    return spellbook.size() - 1;
}
