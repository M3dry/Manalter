#include "item_drops.hpp"
#include "hitbox.hpp"
#include "raylib.h"

ItemDrop::ItemDrop(Vector2 center, Spell&& spell) : hitbox(center, hitbox_radius), item(std::move(spell)) {
}

void ItemDrop::draw_name(std::function<Vector2(Vector3)> to_screen_coords) const {
    Vector2 pos = to_screen_coords((Vector3){hitbox.center.x, 0.0f, hitbox.center.y});
    auto name = get_name().data();
    DrawText(name, pos.x, pos.y, 20, WHITE);
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

void ItemDrops::draw_item_drops() const {
    for (const auto& drop : item_drops) {
        DrawCircle3D((Vector3){drop.hitbox.center.x, 1.0f, drop.hitbox.center.y}, drop.hitbox.radius,
                     (Vector3){1.0f, 0.0f, 0.0f}, 90.0f, RED);
    }
}

#ifdef DEBUG
void ItemDrops::draw_item_drop_names(std::function<Vector2(Vector3)> to_screen_coords) const {
    for (const auto& drop : item_drops) {
        drop.draw_name(to_screen_coords);
    }
}
#endif
