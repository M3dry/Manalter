#include "item_drops.hpp"
#include "particle/effects.hpp"
#include "hitbox.hpp"
#include "raylib.h"
#include "spell.hpp"

effects::Id effect_from_rarity(const Rarity& rarity, const Vector2& center) {
    return effects::push_effect(effect::ItemDrop{ .y = 1.0f }(center, rarity::get_rarity_info(rarity).color));
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

#ifdef DEBUG
void ItemDrops::draw_item_drop_names(std::function<Vector2(Vector3)> to_screen_coords) const {
    for (const auto& drop : item_drops) {
        drop.draw_name(to_screen_coords);
    }
}
#endif
