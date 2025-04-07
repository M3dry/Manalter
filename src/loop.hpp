#pragma once

#include "enemies.hpp"
#include "input.hpp"
#include "raylib.h"

#include "assets.hpp"
#include "enemies_spawner.hpp"
#include "item_drops.hpp"
#include "player.hpp"
#include "ui.hpp"
#include "power_up.hpp"
#include <variant>

struct Loop;

struct Arena {
    struct Playing {
        Playing(Keys& keys);
    };

    struct PowerUpSelection {
        std::array<PowerUp, 3> power_ups;
        std::vector<ui::Button> selections;

        PowerUpSelection(assets::Store& assets, Keys& keys, Vector2 screen);

        void draw(assets::Store& assets, Loop& loop, Arena& arena);
        void update(Arena& arena, Loop& loop);

        void update_buttons(assets::Store& assets, Vector2 screen);
    };

    struct Paused {
        Paused(Keys& keys);

        void draw(assets::Store& assets, Loop& loop);
        void update(Arena& arena, Loop& loop);
    };

    std::variant<Playing, PowerUpSelection, Paused> state;
    Player player;
    Enemies enemies;
    ItemDrops item_drops;

    std::optional<hud::SpellBookUI> spellbook_ui;
    hud::SpellBar spellbar;

    Vector2 mouse_xz;

    Arena(Keys& keys, assets::Store& assets);

    Arena(Arena&) = delete;
    Arena& operator=(Arena&) = delete;

    Arena(Arena&&) noexcept = default;
    Arena& operator=(Arena&&) noexcept = default;

    void draw(assets::Store& assets, Loop& loop);
    void update(Loop& loop);

    template <typename T>
    bool curr_state() const {
        return std::holds_alternative<T>(state);
    }
};

struct Hub {
    Hub(Keys& keys);

    void draw(assets::Store& assets, Loop& loop);
    void update(Loop& loop);
};

struct Main {
    std::optional<ui::Button> play_button;
    std::optional<ui::Button> exit_button;

    Main(assets::Store& assets, Keys& keys, Vector2 screen);

    void draw(assets::Store& assets, Loop& loop);
    void update(Loop& loop);
};

struct SplashScreen {
    void draw(assets::Store& assets, Loop& loop);
    void update(Loop& loop);
};

struct Loop {
    bool screen_updated;
    Vector2 screen;
    Keys keys;
    Mouse mouse;
    assets::Store assets;

    EnemyModels enemy_models;
    Shader skinning_shader;

    // scene isn't Main or SplashScreen -> player_stats has value
    std::variant<Arena, Hub, Main, SplashScreen> scene;
    std::optional<PlayerSave> player_stats;

    Loop(int width, int height);
    void operator()();

    double prev_time = 0.0f;
    double delta_time = 0.0f;
    double accum_time = 0.0f;

    void update();

    ~Loop();
};
