#pragma once

#include "assets.hpp"
#include "hitbox.hpp"
#include "input.hpp"
#include "player.hpp"
#include "raylib.h"
#include "spell.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace ui {
    struct Button {
        enum State {
            Normal,
            Hover,
        };

        std::function<void(State)> draw;
        shapes::Polygon hitbox;

        Button(shapes::Polygon&& poly, std::function<void(State)> draw);

        // draws the button and checks if button was pressed, returns true in that case
        bool update(Mouse& mouse);
    };

    template <typename... Opts> struct Draggable {
        std::function<void(Vector2, Opts...)> draw;
        // default origin
        Vector2 origin;

        // doesn't move, only initiates the dragging
        shapes::Polygon hitbox;

        Draggable(Vector2 origin, shapes::Polygon&& poly, std::function<void(Vector2, Opts...)> draw)
            : draw(draw), origin(origin), hitbox(std::move(poly)) {
        }

        // returns a mouse position at which the component was dropped
        // doesn't call draw if it returns a Vector2
        template <typename... Args> std::optional<Vector2> update(Mouse& mouse, Args&&... args) {
            if (!is_dragged(mouse)) {
                draw(origin, std::forward<Args>(args)...);
                return std::nullopt;
            }

            if (mouse.button_press->released_at) return *mouse.button_press->released_at;

            draw(origin + (mouse.mouse_pos - mouse.button_press->pressed_at), std::forward<Args>(args)...);
            return std::nullopt;
        }

        bool is_dragged(const Mouse& mouse) const {
            if (!mouse.button_press || mouse.button_press->button != Mouse::Button::Left ||
                !check_collision(hitbox, mouse.button_press->pressed_at))
                return false;

            return true;
        }
    };

    template <typename... Opts> struct InventoryPane {
        enum State {
            Normal,
            Hover,
            Drag,
        };

        Vector2 screen;
        Vector2 origin;
        float height;
        // x - initial x padding
        // y - initial y padding
        // z - y padding between items
        Vector3 padding;
        Vector2 item_dims;

        shapes::Polygon scroll_poly;

        uint64_t page_size;
        std::vector<ui::Draggable<const InventoryPane&, uint64_t, State, Opts...>> item_hitboxes;
        std::pair<uint64_t, uint64_t> item_view;
        std::function<void(Vector2, State, uint64_t, Opts...)> draw_item;

        RenderTexture2D drag_defer;
        bool is_deferred = false;

        InventoryPane(Vector2 screen, Vector2 origin, float height, Vector3 padding, Vector2 item_dims,
                      shapes::Polygon&& scroll_box, std::function<void(Vector2, State, uint64_t, Opts...)> draw_item,
                      std::size_t items_size)
            : screen(screen), origin(origin), height(height), padding(padding), item_dims(item_dims),
              scroll_poly(scroll_box), draw_item(draw_item),
              drag_defer(LoadRenderTexture(static_cast<int>(screen.x), static_cast<int>(screen.y))) {
            page_size = static_cast<uint64_t>((height - padding.y) / item_dims.y);
            item_view = {0, std::min(page_size, items_size)};

            item_hitboxes.reserve(page_size);
            for (std::size_t i = 0; i < page_size; i++) {
                item_hitboxes.emplace_back(
                    Vector2{
                        .x = origin.x + padding.x,
                        .y = origin.y + padding.y + i * padding.z + i * item_dims.y,
                    },
                    Rectangle{
                        .x = origin.x + padding.x,
                        .y = origin.y + padding.y + i * padding.z + i * item_dims.y,
                        .width = item_dims.x,
                        .height = item_dims.y,
                    },
                    []<typename... Args>(Vector2 origin, const auto& self, uint64_t item, State state, Args&&... args) {
                        self.draw_item(origin, state, item, std::forward<Args>(args)...);
                    });
            }
        }

        InventoryPane(const InventoryPane&) = delete;
        InventoryPane& operator=(const InventoryPane&) = delete;

        InventoryPane(InventoryPane&& ip) noexcept
            : screen(ip.screen), origin(ip.origin), height(ip.height), padding(ip.padding),
              item_dims(std::move(ip.item_dims)), item_view(ip.item_view), draw_item(std::move(ip.draw_item)),
              drag_defer(ip.drag_defer), is_deferred(ip.is_deferred) {
            ip.drag_defer.texture.id = 0;
        };
        InventoryPane& operator=(InventoryPane&&) noexcept = default;

        ~InventoryPane() {
            if (drag_defer.texture.id != 0) UnloadRenderTexture(drag_defer);
        }

        template <typename... Args>
        std::optional<std::pair<uint64_t, Vector2>> update(Mouse& mouse, std::size_t items_size, Args&&... args) {
            // FIX: scrolling won't work if item_view range is smaller than page_size
            // FIX: also the item_view won't get updated when the items_size decreases
            auto& [first, second] = item_view;
            if (second < page_size) {
                first = 0;
                second = std::min(items_size, page_size);
            }
            if (mouse.wheel_movement != 0 && check_collision(scroll_poly, mouse.mouse_pos) && second >= page_size) {
                if (mouse.wheel_movement < 0) {
                    first += static_cast<uint64_t>(-mouse.wheel_movement);
                    second += static_cast<uint64_t>(-mouse.wheel_movement);

                    if (second > items_size) {
                        first = items_size - page_size;
                        second = items_size;
                    }
                } else if (first >= static_cast<uint64_t>(mouse.wheel_movement)) {
                    first -= static_cast<std::size_t>(mouse.wheel_movement);
                    second -= static_cast<std::size_t>(mouse.wheel_movement);
                } else {
                    first = 0;
                    second = std::min(page_size, items_size);
                }
            }

            is_deferred = false;
            std::optional<std::tuple<uint64_t, Vector2>> ret;
            for (std::size_t item_ix = first; item_ix < second; item_ix++) {
                auto hover = !mouse.button_press && check_collision(item_hitboxes[item_ix - first].hitbox, mouse.mouse_pos);

                auto drag = item_hitboxes[item_ix - first].is_dragged(mouse);
                if (drag) {
                    is_deferred = true;
                    BeginTextureMode(drag_defer);
                    ClearBackground(BLANK);
                }
                if (auto pos = item_hitboxes[item_ix - first].update(mouse, *this, item_ix,
                                                                     drag ? Drag
                                                                     : hover ? Hover
                                                                             : Normal,
                                                                     std::forward<Args>(args)...);
                    pos) {
                    assert(!ret);
                    ret = {item_ix, *pos};
                }
                if (drag) EndTextureMode();
            }

            return ret;
        }

        inline void draw_deffered() {
            if (is_deferred) {
                DrawTexturePro(drag_defer.texture,
                               Rectangle{
                                   .x = 0.0f,
                                   .y = 0.0f,
                                   .width = screen.x,
                                   .height = -screen.y,
                               },
                               Rectangle{
                                   .x = 0.0f,
                                   .y = 0.0f,
                                   .width = screen.x,
                                   .height = screen.y,
                               },
                               Vector2Zero(), 0.0f, WHITE);
            }
        }

        std::optional<uint64_t> dragged(Mouse& mouse) const {
            for (uint64_t i = item_view.first; i < item_view.second; i++) {
                const auto& item = item_hitboxes[i - item_view.first];
                if (check_collision(item.hitbox, mouse.mouse_pos) || item.is_dragged(mouse)) return i;
            }

            return std::nullopt;
        }
    };
}

namespace hud {
    void draw(assets::Store& assets, const Player& player, const SpellBook& spellbook, const Vector2& screen);

    struct SpellBar {
        // dims.xy - top left corner
        // dim.z - height
        int8_t dragged(Vector2 point, Vector3 dims, uint8_t unlocked_count);
        void draw(Vector3 dims, assets::Store& assets, const SpellBook& spellbook, std::span<std::size_t> equipped);
    };
}
