#include "player.hpp"
#include "spell_caster.hpp"
#include "utility.hpp"

const Vector3 Player::camera_offset = (Vector3){60.0f, 140.0f, 0.0f};
const float Player::model_scale = 0.2f;

Player::Player(Vector3 position)
    : prev_position(position), position(position), interpolated_position(position),
      model(LoadModel("./assets/player/player.glb")), animations(nullptr),
      hitbox((Vector2){position.x, position.z}, 8.0f) {
    camera.position = camera_offset;
    camera.target = position;
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 90.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    BoundingBox mesh_bb = GetMeshBoundingBox(model.meshes[0]);
    Vector3 min = Vector3Scale(mesh_bb.min, model_scale);
    Vector3 max = Vector3Scale(mesh_bb.max, model_scale);

    model.transform = MatrixMultiply(model.transform, MatrixRotateX(std::numbers::pi / 2.0f));

    animations = LoadModelAnimations("./assets/player/player.glb", &animationsCount);
    UpdateModelAnimation(model, animations[animationIndex], animationCurrent);
}

void Player::update_interpolated_pos(double accum_time) {
    interpolated_position = Vector3Lerp(prev_position, position, accum_time * TICKS);
    prev_position = interpolated_position;
    camera.target = interpolated_position;
    camera.position = Vector3Lerp(camera.position, Vector3Add(camera_offset, interpolated_position), 1.0f);
}

void Player::update(Vector2 movement, float new_angle) {
    int lastIndex = animationIndex;
    if (movement.x == 0 && movement.y == 0) {
        animationIndex = 0;
    } else {
        prev_position = position;
        position.x += movement.x;
        position.z += movement.y;
        angle = new_angle;
        hitbox.update(movement);

        animationIndex = 2;
    }

    if (lastIndex != animationIndex) {
        animationCurrent = 0;
    } else {
        animationCurrent = (animationCurrent + 3) % animations[animationIndex].frameCount;
    }
    UpdateModelAnimation(model, animations[animationIndex], animationCurrent);
}

void Player::draw_model() const {
    DrawModelEx(model, interpolated_position, (Vector3){0.0f, 1.0f, 0.0f}, angle,
                (Vector3){model_scale, model_scale, model_scale}, WHITE);

#ifdef DEBUG
    DrawSphere((Vector3){hitbox.center.x, 1.0f, hitbox.center.y}, 1.0f, BLUE);
    hitbox.draw_3D(RED, 1.0f);
#endif
}

Player::~Player() {
    UnloadModel(model);
    UnloadModelAnimations(animations, animationsCount);
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

        if (spell.current_cooldown == 0) continue;
        spell.current_cooldown--;
    }

    tick_counter++;
}

void PlayerStats::cast_equipped(int idx, const Vector2& player_position, const Vector2& mouse_pos) {
    if (idx >= 10 || idx >= max_spells || equipped_spells[idx] == UINT32_MAX) return;

    auto spell_id = equipped_spells[idx];
    Spell& spell = spellbook[spell_id];
    if (mana < spell.manacost || spell.current_cooldown > 0) return;

    mana -= spell.manacost;
    spell.current_cooldown = spell.cooldown;

    caster::cast(spell_id, spell, player_position, mouse_pos);
}
