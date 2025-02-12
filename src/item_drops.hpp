#pragma once

#include "hitbox.hpp"
#include "spell.hpp"
#include <algorithm>
#include <functional>
#include <string_view>
#include <variant>

class ItemDrop {
  public:
    static constexpr float hitbox_radius = 5.0f;

    using Item = std::variant<Spell>;
    Item item;
    shapes::Circle hitbox;

    ItemDrop(Vector2 center, Spell&& spell);

    ItemDrop(ItemDrop&&) noexcept = default;
    ItemDrop& operator=(ItemDrop&&) noexcept = default;

    void draw_name(std::function<Vector2(Vector3)> to_screen_coords) const;
    std::string_view get_name() const;

    static ItemDrop random(uint16_t level, const Vector2& pos);
};

struct ItemDrops {
    std::vector<ItemDrop> item_drops;

    template <typename... Args> void add_item_drop(Args... args) {
        item_drops.emplace_back(std::forward<Args>(args)...);
    }

    void draw_item_drops() const;

    void pickup(const Shape auto& shape, auto handler) {
        auto [first, last] = std::ranges::remove_if(item_drops, [&](auto& drop) -> bool {
            if (!check_collision(shape, drop.hitbox)) return false;

            std::visit(handler, std::move(drop.item));
            return true;
        });
        item_drops.erase(first, last);
    };

#ifdef DEBUG
    void draw_item_drop_names(std::function<Vector2(Vector3)> to_screen_coords) const;
#endif
};
