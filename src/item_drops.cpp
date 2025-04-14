#include "item_drops.hpp"
#include "effects.hpp"
#include "hitbox.hpp"
#include "raylib.h"
#include "spell.hpp"
#include <limits>
#include <print>

effects::Id effect_from_rarity(const Rarity& rarity, const Vector2& center) {
    Color init_color = rarity::get_rarity_info(rarity).color;
    auto eff = effect::Plosion{
        .type = effect::Plosion::Ex,
        .radius = 2.0f,
        .particle_count = 200,
        .max_emit = std::numeric_limits<std::size_t>::max(),
        .emit_rate = 50000,
        .particle_size_scale = 0.1f,
        .floor_y = 0.0f,
        .lifetime = {0.1f, 0.3f},
        .velocity_scale = {30.0f, 50.0f},
        .acceleration = {100.0f, -50.0f, 100.0f},
        .color = {{init_color, 35.0f}, {WHITE, 200.0f}},
    };
    auto system = eff(center);
    system.reset_on_done = std::numeric_limits<std::size_t>::max();
    return effects::push_effect(std::move(system));
}

ItemDrop::ItemDrop(Vector2 center, Spell&& spell) : item(std::move(spell)), hitbox(center, hitbox_radius) {
    make_effect();
}

ItemDrop::ItemDrop(uint32_t level, const Vector2& center) : item(Spell::random(level)), hitbox(center, hitbox_radius) {
    make_effect();
}

ItemDrop::~ItemDrop() {
    effects::pop_effect(effect_id);
}

void ItemDrop::draw_name(std::function<Vector2(Vector3)> to_screen_coords) const {
    Vector2 pos = to_screen_coords((Vector3){hitbox.center.x, 0.0f, hitbox.center.y});
    auto name = get_name().data();
    DrawText(name, static_cast<int>(pos.x), static_cast<int>(pos.y), 20, WHITE);
}

std::string_view ItemDrop::get_name() const {
    return std::visit(
        [](auto&& arg) -> std::string_view {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, Spell>) {
                return arg.get_spell_info().name;
            }
        },
        item);
}

ItemDrop ItemDrop::random(uint32_t level, const Vector2& center) {
    return ItemDrop(center, Spell::random(level));
}

void ItemDrop::make_effect() {
    std::visit(
        [&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::same_as<T, Spell>) {
                effect_id = effect_from_rarity(arg.rarity, hitbox.center);
            }
        },
        item);
}

void ItemDrops::draw_item_drops(const Vector3& offset) const {
    for (const auto& drop : item_drops) {
        DrawCircle3D((Vector3){drop.hitbox.center.x + offset.x, 1.0f + offset.y, drop.hitbox.center.y + offset.z},
                     drop.hitbox.radius, (Vector3){1.0f, 0.0f, 0.0f}, 90.0f, RED);
    }
}

#ifdef DEBUG
void ItemDrops::draw_item_drop_names(std::function<Vector2(Vector3)> to_screen_coords) const {
    for (const auto& drop : item_drops) {
        drop.draw_name(to_screen_coords);
    }
}
#endif
