#pragma once

#include "enemies.hpp"
#include "input.hpp"
#include "raylib.h"

#include "assets.hpp"
#include "enemies_spawner.hpp"
#include "item_drops.hpp"
#include "player.hpp"
#include "power_up.hpp"
#include "ui.hpp"
#include <variant>

struct Loop;

struct Arena {
    struct Playing {
        Playing(Keys& keys);
    };

    struct PowerUpSelection {
        std::array<PowerUp, 3> power_ups;
        std::vector<ui::Button> selections;

        PowerUpSelection(Loop& loop);

        void draw(Arena& arena, Loop& loop);
        void update(Arena& arena, Loop& loop);

        void update_buttons(assets::Store& assets, Vector2 screen);
    };

    struct Paused {
        std::optional<ui::Button> continue_button;
        std::optional<ui::Button> quit_button;

        Paused(Loop& Loop);

        void draw(Arena& arena, Loop& loop);
        void update(Arena& arena, Loop& loop);
    };

    struct SoulPortal {
        double time_remaining;
        shapes::Circle hitbox;
    };

    struct SpellBookUI {
        Vector2 spellbook_dims;
        ui::InventoryPane<assets::Store&, std::span<uint64_t>, const SpellBook&> inventory_pane;

        template <typename... Args>
        SpellBookUI(Vector2 dims, Args&&... args) : spellbook_dims(dims), inventory_pane(std::forward<Args>(args)...) {
        }

        template <typename... Args>
        decltype(auto) update(Args&&... args) {
            return inventory_pane.update(std::forward<Args>(args)...);
        }
    };

    std::variant<Playing, PowerUpSelection, Paused> state;
    Player player;
    Enemies enemies;
    ItemDrops item_drops;

    uint64_t souls = 0;
    double game_time = 0.0;
    double to_next_soul_portal;
    std::optional<SoulPortal> soul_portal;
    Model soul_portal_arrow;

    std::optional<SpellBookUI> spellbook_ui;
    hud::SpellBar spellbar;

    Vector2 mouse_xz;

    Arena(Loop& loop);

    Arena(Arena&) = delete;
    Arena& operator=(Arena&) = delete;

    Arena(Arena&&) noexcept = default;
    Arena& operator=(Arena&&) noexcept = default;

    ~Arena();

    void draw(Loop& loop);
    void update(Loop& loop);

    void incr_time(double delta);

    template <typename T> inline bool curr_state() const {
        return std::holds_alternative<T>(state);
    }
};

struct Hub {
    std::optional<ui::Button> start_button;

    Rectangle stash_rec;
    std::optional<ui::InventoryPane<assets::Store&, const SpellBook&>> stash;

    Rectangle spellbook_rec;
    std::optional<ui::InventoryPane<assets::Store&, const SpellBook&>> spellbook;

    Hub(Loop& loop);

    void draw(Loop& loop);
    void update(Loop& loop);
};

struct Main {
    std::optional<ui::Button> play_button;
    std::optional<ui::Button> exit_button;

    Main(Loop& loop);

    void draw(Loop& loop);
    void update(Loop& loop);
};

struct SplashScreen {
    void draw(Loop& loop);
    void update(Loop& loop);
};

struct Loop {
    bool screen_updated;
    Vector2 screen;
    Keys keys;
    Mouse mouse;
    Mouse dummy_mouse = Mouse(Mouse::Dummy{});
    assets::Store assets;

    EnemyModels enemy_models;
    Shader skinning_shader;

    // scene isn't Main or SplashScreen -> player_stats has value
    std::variant<Arena, Hub, Main, SplashScreen> scene;
    std::optional<PlayerSave> player_save;

    Loop(int width, int height);
    void operator()();

    double prev_time = 0;
    double delta_time = 0;
    double accum_time = 0;

    void update();

    ~Loop();
};
