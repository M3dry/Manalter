#pragma once

#include "particle/effects.hpp"
#include "hitbox.hpp"
#include "spell.hpp"
#include <functional>
#include <string_view>
#include <variant>

class ItemDrop {
  public:
    static constexpr float hitbox_radius = 15.0f;

    using Item = std::variant<Spell>;
    Item item;
    shapes::Circle hitbox;

    effects::Id effect_id;

    ItemDrop(Vector2 center, Spell&& spell);
    ItemDrop(uint32_t level, const Vector2& center);

    ItemDrop(const ItemDrop&) = delete;
    ItemDrop& operator=(const ItemDrop&) = delete;

    ItemDrop(ItemDrop&& i) noexcept : item(std::move(i.item)), hitbox(std::move(i.hitbox)), effect_id(i.effect_id) {
        i.effect_id = {-1, -1};
    };
    ItemDrop& operator=(ItemDrop&& i) noexcept {
        if (this != &i) {
            item = std::move(i.item);
            hitbox = std::move(i.hitbox);
            effect_id = i.effect_id;
            i.effect_id = {-1, -1};
        }

        return *this;
    };

    ~ItemDrop();

    void draw_name(std::function<Vector2(Vector3)> to_screen_coords) const;
    std::string_view get_name() const;

    static ItemDrop random(uint32_t level, const Vector2& center);

  private:
    void make_effect();
};

struct ItemDrops {
    std::vector<ItemDrop> item_drops;

    template <typename... Args> void add_item_drop(Args&&... args) {
        item_drops.emplace_back(std::forward<Args>(args)...);
    }

    void pickup(const Shape auto& shape, auto handler) {
        std::size_t i = 0;
        while (i < item_drops.size()) {
            if (!check_collision(shape, item_drops[i].hitbox)) {
                i++;
                continue;
            }

            std::visit(handler, std::move(item_drops[i].item));
            std::swap(item_drops[i], item_drops.back());
            item_drops.pop_back();
        }
    };

#ifdef DEBUG
    void draw_item_drop_names(std::function<Vector2(Vector3)> to_screen_coords) const;
#endif
};
